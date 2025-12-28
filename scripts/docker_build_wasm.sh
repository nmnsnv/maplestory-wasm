#!/bin/bash
cd "$(dirname "$0")/.."
docker compose run --rm wasm-builder bash -c "python3 patch_system/scripts/sync.py && python3 patch_system/scripts/apply_patches.py && ./scripts/build_wasm.sh"
