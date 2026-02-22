#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

# wasm-builder joins an external Docker network, so ensure it exists for one-off builds.
docker network create maplestory-network >/dev/null 2>&1 || true

if [ -d "src/client" ] && [ -f "src/client/CMakeLists.txt" ]; then
  if [ -f "build/CMakeCache.txt" ] && ! grep -q '^CMAKE_HOME_DIRECTORY:INTERNAL=/app/src/client$' "build/CMakeCache.txt"; then
    echo "Detected host-generated CMake cache. Cleaning stale cache files..."
    rm -rf build/CMakeCache.txt build/CMakeFiles
  fi
  docker compose run --rm wasm-builder bash -c './scripts/build_wasm.sh "$@"' _ "$@"
else
  docker compose run --rm wasm-builder bash -c \
    'python3 patch_system/scripts/sync.py && python3 patch_system/scripts/apply_patches.py && ./scripts/build_wasm.sh "$@"' \
    _ "$@"
fi

# Fix compile_commands.json paths for host-side tooling.
if [ -f "build/compile_commands.json" ]; then
  echo "Fixing paths in compile_commands.json..."
  python3 -c "import os,sys; p=sys.argv[1]; c=open(p).read().replace('/app', os.getcwd()); open(p, 'w').write(c)" "build/compile_commands.json"
fi

# Fix paths in .rsp files when present.
if [ -d "build" ]; then
  echo "Fixing paths in .rsp files..."
  while IFS= read -r -d '' rsp; do
    python3 -c "import os,sys; p=sys.argv[1]; c=open(p).read().replace('/app', os.getcwd()); open(p, 'w').write(c)" "$rsp"
  done < <(find build -name "*.rsp" -print0)
fi
