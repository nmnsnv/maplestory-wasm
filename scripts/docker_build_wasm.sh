#!/bin/bash
cd "$(dirname "$0")/.."
docker compose run --rm wasm-builder bash -c "python3 patch_system/scripts/sync.py && python3 patch_system/scripts/apply_patches.py && ./scripts/build_wasm.sh"

# Fix compile_commands.json paths
if [ -f "build/compile_commands.json" ]; then
    echo "Fixing paths in compile_commands.json..."
    python3 -c "import os, sys; content = open('build/compile_commands.json').read().replace('/app', os.getcwd()); open('build/compile_commands.json', 'w').write(content)"
fi

# Fix paths in .rsp files
echo "Fixing paths in .rsp files..."
find build -name "*.rsp" -print0 | xargs -0 -I {} python3 -c "import os, sys; content = open('{}').read().replace('/app', os.getcwd()); open('{}', 'w').write(content)"
