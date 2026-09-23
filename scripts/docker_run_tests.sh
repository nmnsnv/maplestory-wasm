#!/usr/bin/env bash
set -euo pipefail

TEST_REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# A standalone native runner needs neither the web stack nor the game network.
docker build -f "$TEST_REPO_DIR/docker/tests.Dockerfile" -t maplestory-native-tests "$TEST_REPO_DIR/docker"
TEST_DOCKER_ARGS=(--rm --network none --user "$(id -u):$(id -g)"
    -e JOURNEY_TEST_CONTAINER=1 --mount "type=bind,source=$TEST_REPO_DIR,target=/app" -w /app)
TEST_DOCKER_ASSETS="${TEST_ASSET_DIR:-$TEST_REPO_DIR/assets}"
if [[ -d "$TEST_DOCKER_ASSETS" ]]; then
    TEST_DOCKER_ASSETS="$(cd "$TEST_DOCKER_ASSETS" && pwd)"
    TEST_DOCKER_ARGS+=(--mount "type=bind,source=$TEST_DOCKER_ASSETS,target=/app/assets,readonly")
fi
docker run "${TEST_DOCKER_ARGS[@]}" maplestory-native-tests ./scripts/run_tests.sh "$@"
