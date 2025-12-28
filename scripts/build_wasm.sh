#!/bin/bash

# --- THE FIX ---
# 1. Get the directory where this script actually lives
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 2. Change directory to the script's location immediately.
# This ensures that all relative paths (like ../) work from this point onward.
cd "$SCRIPT_DIR"
# ---------------

# Ensure we exit on error
set -e

# Parse command-line arguments
DEBUG_SYMBOLS_FLAG="OFF"
J_ARG=36
for arg in "$@"; do
    case $arg in
        --debug|-g)
            DEBUG_SYMBOLS_FLAG="ON"
            shift
            ;;
        --jobs|-j)
            shift
            J_ARG=$1
            shift # Shift again to consume the number
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --debug, -g    Enable debug symbols (-g flag)"
            echo "  --jobs, -j     Number of jobs to use (default: 1)"
            echo "  --help, -h     Show this help message"
            exit 0
            ;;
    esac
done

# Allow environment variable to override
if [ "$DEBUG_SYMBOLS_FLAG" = "OFF" ] && [ "$DEBUG_SYMBOLS" = "ON" ]; then
    DEBUG_SYMBOLS_FLAG="ON"
fi

# Define directories relative to SCRIPT_DIR
# Adjust the number of ".." based on where this script sits (e.g., in a /scripts folder)
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
SOURCE_DIR="$ROOT_DIR/src/client"

echo "Building Wasm client..."
echo "Root:   $ROOT_DIR"
echo "Build:  $BUILD_DIR"
echo "Source: $SOURCE_DIR"

# Create and enter build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Determine Build Type
# Always use RelWithDebInfo to balance performance (-O2) and debuggability (-g)
BUILD_TYPE="Release"

# Run CMake and Make
emcmake cmake "$SOURCE_DIR" -DDEBUG_SYMBOLS=$DEBUG_SYMBOLS_FLAG -DCMAKE_BUILD_TYPE=$BUILD_TYPE
emmake make -j$J_ARG

echo "Build finished successfully."