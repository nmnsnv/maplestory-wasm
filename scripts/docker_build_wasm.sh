#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
DOCKER_SOURCE_DIR="/app/src/client"
CACHE_FILE="$BUILD_DIR/CMakeCache.txt"

cd "$ROOT_DIR"

clean_stale_cmake_state() {
  echo "Cleaning incompatible CMake metadata for the Docker build..."
  rm -rf \
    "$BUILD_DIR/CMakeCache.txt" \
    "$BUILD_DIR/CMakeFiles" \
    "$BUILD_DIR/Makefile" \
    "$BUILD_DIR/cmake_install.cmake" \
    "$BUILD_DIR/compile_commands.json"
}

# wasm-builder joins an external Docker network, so ensure it exists for one-off builds.
docker network create maplestory-network >/dev/null 2>&1 || true

if [ -f "$CACHE_FILE" ]; then
  CACHE_SOURCE_DIR="$(grep '^CMAKE_HOME_DIRECTORY:INTERNAL=' "$CACHE_FILE" | cut -d= -f2- || true)"

  if [ "$CACHE_SOURCE_DIR" != "$DOCKER_SOURCE_DIR" ]; then
    clean_stale_cmake_state
  fi
fi

# docker-compose.yml bind-mounts the repo into /app, so /app/build persists
# to the host build/ directory and Docker rebuilds can reuse object files.
docker compose run --rm wasm-builder ./scripts/build_wasm.sh "$@"
