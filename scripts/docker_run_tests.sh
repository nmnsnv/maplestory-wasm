#!/bin/bash

# Build and run the native host unit tests inside the Docker builder image
# (the same `wasm-builder` service used for WASM builds). Extra arguments are
# forwarded to ctest, e.g.:
#   ./scripts/docker_run_tests.sh -R Keyboard

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$ROOT_DIR"

# wasm-builder joins an external Docker network, so ensure it exists for one-off runs.
docker network create maplestory-network >/dev/null 2>&1 || true

# --build keeps the builder image in sync with docker/builder.Dockerfile (so the
# native toolchain + FreeType test deps are present); layer caching makes this a
# no-op once built. Use a dedicated build dir so the Linux container's CMake
# cache never collides with a host (e.g. macOS) build-tests/ tree at the same path.
docker compose run --build --rm \
  -e TESTS_BUILD_DIR=build-tests-docker \
  wasm-builder ./scripts/run_tests.sh "$@"
