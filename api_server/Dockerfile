# 本番環境デプロイ用
#
# - base イメージとして slim 版を使用
# - poetry install の際に dev-dependencies をインストールしない
# - uvicorn ではなく gunicorn を使用

FROM python:3.12.2-slim-bookworm as base

WORKDIR /src

ENV POETRY_VERSION=1.8.3
ENV PYTHONUNBUFFERED=1

# pip のアップグレードと poetry のインストール
RUN pip install --no-cache-dir -U pip \
        && pip install --no-cache-dir poetry==${POETRY_VERSION} \
        && poetry config virtualenvs.create false

# poetryの定義ファイルをコピー
COPY pyproject.toml* poetry.lock* ./

# dev-dependenciesを除いてライブラリをインストール
RUN poetry install --without dev

# ソースコードをコピー
COPY ./app /src/app

# gunicornのサーバーを立ち上げる
CMD ["poetry", "run", "gunicorn", "-k", "uvicorn.workers.UvicornWorker", "app.main:api", "--bind", "0.0.0.0:8000", "--workers", "2", "--access-logfile", "-"]

EXPOSE 8000
