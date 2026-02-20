#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$ROOT_DIR/scripts/wz-converter/docker-compose.yml"

if ! compgen -G "$ROOT_DIR/wz/*.wz" > /dev/null; then
  echo "No WZ files found in: $ROOT_DIR/wz" >&2
  exit 1
fi

exec docker compose -f "$COMPOSE_FILE" run --rm wz-converter
