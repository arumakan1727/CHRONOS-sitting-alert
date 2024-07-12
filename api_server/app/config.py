from pydantic import HttpUrl
from pydantic_settings import BaseSettings, SettingsConfigDict

# os.getenv は使わずにこの EnvConfig オブジェクト経由で環境変数を読み込む。


class EnvConfig(BaseSettings):
    """$ENV_FILE と .env と環境変数を順にバリデーション付きで読み込む。

    環境変数 ENV_FILE が未定義の場合はデフォルト値として ".env.local" を使う。

    $ENV_FILE ファイルが存在しなくても環境変数が適切にセットされていればエラーにならない。
    参考: https://docs.pydantic.dev/latest/concepts/pydantic_settings/#dotenv-env-support
    """

    model_config = SettingsConfigDict(
        frozen=True,
        extra="forbid",
        env_file=[".env"],
    )

    # フィールド名は全て大文字である必要はないが、
    # .envとの対応のわかりやすさを優先してスクリーミングスネークケースで定義している。

    INFLUXDB_BUCKET: str
    INFLUXDB_ORG: str
    INFLUXDB_TOKEN: str
    INFLUXDB_URL: HttpUrl


cfg = EnvConfig()  # type:ignore (Aruguments missing)
