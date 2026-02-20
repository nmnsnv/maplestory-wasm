#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="/work"
WZ_DIR="$ROOT_DIR/wz"
NX_DIR="$ROOT_DIR/nxFiles"
CACHE_DIR="$ROOT_DIR/.cache/tools"
SRC_DIR="$CACHE_DIR/NoLifeWzToNx"
BIN_DIR="$CACHE_DIR/bin"
BIN_PATH="$BIN_DIR/NoLifeWzToNx"

REPO_URL="${NO_LIFE_WZ_TO_NX_REPO:-https://github.com/ryantpayton/NoLifeWzToNx}"
REPO_REF="${NO_LIFE_WZ_TO_NX_REF:-master}"
SKIP_LIST_WZ="${SKIP_LIST_WZ:-1}"
MODE="${MODE:-client}" # client|server
LZ4HC="${LZ4HC:-0}"

mkdir -p "$CACHE_DIR" "$BIN_DIR" "$NX_DIR"

shopt -s nullglob
wz_files=("$WZ_DIR"/*.wz)
if [[ ${#wz_files[@]} -eq 0 ]]; then
  echo "No WZ files found in: $WZ_DIR" >&2
  exit 1
fi

if [[ ! -d "$SRC_DIR/.git" ]]; then
  echo "Cloning NoLifeWzToNx into $SRC_DIR"
  git clone "$REPO_URL" "$SRC_DIR"
fi

pushd "$SRC_DIR" >/dev/null

echo "Fetching latest from origin"
git fetch --all --tags --prune

echo "Checking out ref: $REPO_REF"
git checkout "$REPO_REF"
if git rev-parse --verify --quiet "origin/$REPO_REF" >/dev/null; then
  git reset --hard "origin/$REPO_REF"
fi

# Upstream currently includes <filesystem> but aliases std::experimental::filesystem.
# Patch in-container for modern GCC toolchains.
if grep -q "std::experimental::filesystem" NoLifeWzToNx.cpp; then
  sed -i 's/std::experimental::filesystem/std::filesystem/g' NoLifeWzToNx.cpp
fi
# Harden string conversion: malformed decoded text should not abort conversion.
if grep -q "return convert.to_bytes(ptr, ptr + p_str.size());" NoLifeWzToNx.cpp; then
  perl -0777 -i -pe "s@return convert\\.to_bytes\\(ptr, ptr \\+ p_str\\.size\\(\\)\\);@try { return convert.to_bytes(ptr, ptr + p_str.size()); } catch (...) { std::string fallback; fallback.reserve(p_str.size()); for (auto ch : p_str) fallback.push_back(ch < 0x80 ? static_cast<char>(ch) : '?'); return fallback; }@g" NoLifeWzToNx.cpp
fi
# Harden bitmap processing: some modern WZ bitmaps fail strict size checks.
# Fallback to blank bitmap data instead of aborting the entire conversion.
if grep -q "throw std::runtime_error(\"halp!\");" NoLifeWzToNx.cpp; then
  perl -0777 -i -pe "s@if \\(check != pixels \\* 4\\) \\{\\s*std::cerr << \"Size mismatch: \" << std::dec << width << \",\" << height << \",\" << decompressed << \",\" << f1 << \",\" << f2 << std::endl;\\s*throw std::runtime_error\\(\"halp!\"\\);\\s*\\}@if (check != pixels * 4) { std::cerr << \"Size mismatch: \" << std::dec << width << \",\" << height << \",\" << decompressed << \",\" << f1 << \",\" << f2 << std::endl; f1 = 2; f2 = 0; decompressed = size; std::fill(output.begin(), output.begin() + size, '\\\\0'); }@g" NoLifeWzToNx.cpp
fi

echo "Building converter binary (g++)"
g++ -std=c++17 -O2 -Iincludes/libsquish \
  NoLifeWzToNx.cpp Keys.cpp includes/libsquish/*.cpp \
  -lz -llz4 -o "$BIN_PATH"

popd >/dev/null

if [[ ! -x "$BIN_PATH" ]]; then
  echo "Failed to build converter: $BIN_PATH not found" >&2
  exit 1
fi

echo "Using converter binary: $BIN_PATH"

# Clean stale outputs in target dir
rm -f "$NX_DIR"/*.nx

for wz in "${wz_files[@]}"; do
  base="$(basename "$wz")"
  if [[ "$base" == "List.wz" && "$SKIP_LIST_WZ" == "1" ]]; then
    echo "Skipping $base (non-PKG1 metadata file)"
    continue
  fi

  echo "Converting $base"
  args=("$wz")
  if [[ "$MODE" == "server" ]]; then
    args+=("-s")
  else
    args+=("-c")
  fi
  if [[ "$LZ4HC" == "1" ]]; then
    args+=("-h")
  fi

  "$BIN_PATH" "${args[@]}"

done

# Move produced .nx files (tool writes next to input WZ)
for nx in "$WZ_DIR"/*.nx; do
  [ -f "$nx" ] || continue
  cp -f "$nx" "$NX_DIR"/
done

echo "Done. NX files in $NX_DIR:"
ls -lh "$NX_DIR"/*.nx
