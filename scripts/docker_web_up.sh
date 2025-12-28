#!/bin/bash
cd "$(dirname "$0")/.."
docker compose up "$@" html-server ws-proxy assets-server
