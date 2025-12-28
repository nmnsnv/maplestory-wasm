#!/bin/bash
cd "$(dirname "$0")/../src/server"
docker compose up "$@"
