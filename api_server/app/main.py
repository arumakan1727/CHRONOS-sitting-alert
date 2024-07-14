from typing import Literal
from fastapi import FastAPI
from fastapi.responses import PlainTextResponse
from pydantic import BaseModel, Field
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
from app.config import cfg

api = FastAPI()

influxdb = InfluxDBClient(
    url=str(cfg.INFLUXDB_URL),
    token=cfg.INFLUXDB_TOKEN,
    org=cfg.INFLUXDB_ORG,
)
influxdb_write_api = influxdb.write_api(write_options=SYNCHRONOUS)
influxdb_query_api = influxdb.query_api()


@api.get("/health")
def get_health() -> PlainTextResponse:
    return PlainTextResponse("OK")


@api.get("/health/influxdb")
def fetch_influxdb_for_healthcheck() -> PlainTextResponse:
    influxdb_query_api.query(
        f'from(bucket:"{cfg.INFLUXDB_BUCKET}") |> range(start: -10s)'
    )
    return PlainTextResponse("Fetching data from InfxluDB was success")


class PostSittingStatusRequest(BaseModel):
    sitting_pressure: int = Field(ge=0, lt=1024)
    is_sitting: bool
    sitting_check_window_data_points: int
    window_sitting_count: int


PostSittingStatusResponse = Literal["OK"]


@api.post("/sitting/status", response_class=PlainTextResponse)
def post_sitting_status(payload: PostSittingStatusRequest) -> PostSittingStatusResponse:
    print(f"{payload=}")
    pt = (
        Point("sitting_state")
        .field("pressure", payload.sitting_pressure)
        .field("is_sitting", payload.is_sitting)
        .field("window_sitting_count", payload.window_sitting_count)
    )
    influxdb_write_api.write(
        bucket=cfg.INFLUXDB_BUCKET, org=cfg.INFLUXDB_ORG, record=pt
    )
    return "OK"
