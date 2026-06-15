#!/bin/bash

# Build and run the native host unit tests (Plan 00 / Plan 07).
#
# Runs the same way on the host or inside the Docker builder image. Requires a
# native C++17 compiler, CMake, and FreeType (located via pkg-config). On macOS:
# `brew install freetype`. Inside Docker use ./scripts/docker_run_tests.sh.
#
# Any extra arguments are forwarded to ctest, e.g.:
#   ./scripts/run_tests.sh -R Keyboard          # run matching tests only
#
# Environment overrides:
#   TESTS_BUILD_DIR  build dir name under src/client (default: build-tests)
#   JOBS             parallel build jobs (default: 4)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
SOURCE_DIR="$ROOT_DIR/src/client"
BUILD_DIR="$SOURCE_DIR/${TESTS_BUILD_DIR:-build-tests}"
JOBS="${JOBS:-4}"

echo "Building native unit tests..."
echo "Source: $SOURCE_DIR"
echo "Build:  $BUILD_DIR"

cmake -S "$SOURCE_DIR" -B "$BUILD_DIR" -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD_DIR" --target tests -j"$JOBS"

# Run from the build dir so the Settings file the client writes on startup lands
# there rather than in the source tree.
cd "$BUILD_DIR"
ctest --output-on-failure "$@"
