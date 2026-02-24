#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
PATCH_DIR="${ROOT_DIR}/patches/nlnx"
NLNX_DIR="${ROOT_DIR}/src/nlnx"

cd "$ROOT_DIR"
echo "[sync] Updating submodules"
git submodule update --init --recursive

if [[ ! -d "$PATCH_DIR" ]]; then
  echo "[sync] No patches/nlnx directory found. Done."
  exit 0
fi

shopt -s nullglob
patches=("$PATCH_DIR"/*.patch)
if [[ ${#patches[@]} -eq 0 ]]; then
  echo "[sync] No *.patch files in patches/nlnx. Done."
  exit 0
fi

for patch in "${patches[@]}"; do
  rel_patch="${patch#${ROOT_DIR}/}"
  echo "[sync] Applying ${rel_patch}"
  if git -C "$NLNX_DIR" apply --check "$patch"; then
    git -C "$NLNX_DIR" apply "$patch"
    echo "[sync] Applied ${rel_patch}"
  elif git -C "$NLNX_DIR" apply -R --check "$patch"; then
    echo "[sync] Already applied ${rel_patch}"
  else
    echo "[sync] Failed to apply ${rel_patch}" >&2
    exit 1
  fi
done

echo "[sync] Done"
