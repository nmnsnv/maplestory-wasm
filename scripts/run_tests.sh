#!/usr/bin/env bash
set -euo pipefail

TEST_REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_WITH_ASSETS=OFF
TEST_WITH_SANITIZERS=OFF
TEST_BUILD_TYPE=Debug
TEST_JOBS=4

usage() {
    cat <<'EOF'
Usage: ./scripts/run_tests.sh [OPTIONS] [-- CTEST_OPTIONS...]

  --assets          Include tests using local read-only NX assets
  --sanitize        Enable AddressSanitizer and UndefinedBehaviorSanitizer
  --release         Build optimized tests; doctest assertions remain enabled
  --jobs N          Parallel build and test jobs (default: 4)
  --help            Show this help

Examples:
  ./scripts/run_tests.sh
  ./scripts/run_tests.sh -- -N
  ./scripts/run_tests.sh -- -R packet
  ./scripts/run_tests.sh --assets -- -L assets

TEST_ASSET_DIR may select an existing NX directory. Reports and diagnostics
are written below build/tests/. Extra arguments are passed directly to CTest.
EOF
}

while (($#)); do
    case "$1" in
        --assets) TEST_WITH_ASSETS=ON; shift ;;
        --sanitize) TEST_WITH_SANITIZERS=ON; shift ;;
        --release) TEST_BUILD_TYPE=Release; shift ;;
        --jobs|-j)
            if (($# < 2)) || [[ ! "$2" =~ ^[1-9][0-9]*$ ]]; then
                echo "--jobs requires a positive integer" >&2
                exit 2
            fi
            TEST_JOBS="$2"; shift 2 ;;
        --help|-h) usage; exit 0 ;;
        --) shift; break ;;
        *) echo "Unknown option: $1 (use -- before CTest options)" >&2; exit 2 ;;
    esac
done

TEST_VARIANT="native-${TEST_BUILD_TYPE}"
if [[ "$TEST_WITH_ASSETS" == ON ]]; then TEST_VARIANT+="-assets"; fi
if [[ "$TEST_WITH_SANITIZERS" == ON ]]; then TEST_VARIANT+="-sanitized"; fi
TEST_BUILD_DIR="$TEST_REPO_DIR/build/tests/$TEST_VARIANT"
TEST_CMAKE_ARGS=(
    -DTEST_ASSETS="$TEST_WITH_ASSETS"
    -DTEST_SANITIZERS="$TEST_WITH_SANITIZERS"
    -DCMAKE_BUILD_TYPE="$TEST_BUILD_TYPE"
    -DTEST_ASSET_DIR="${TEST_ASSET_DIR:-$TEST_REPO_DIR/assets}"
)

# Separate host and container caches; CMake caches contain absolute tool paths.
if [[ "${JOURNEY_TEST_CONTAINER:-0}" == 1 ]]; then
    TEST_BUILD_DIR="$TEST_REPO_DIR/build/tests/docker-${TEST_VARIANT}"
    TEST_CMAKE_ARGS+=(-DFETCHCONTENT_SOURCE_DIR_DOCTEST=/opt/doctest)
fi

mkdir -p "$TEST_BUILD_DIR/reports"
# Keep a previous successful report from masquerading as the current run.
rm -f "$TEST_BUILD_DIR/reports/junit.xml"
cmake -S "$TEST_REPO_DIR/tests" -B "$TEST_BUILD_DIR" "${TEST_CMAKE_ARGS[@]}"
cmake --build "$TEST_BUILD_DIR" --parallel "$TEST_JOBS"
UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}" \
    ctest --test-dir "$TEST_BUILD_DIR" --parallel "$TEST_JOBS" \
    --output-on-failure --no-tests=error \
    --output-junit "$TEST_BUILD_DIR/reports/junit.xml" "$@"
