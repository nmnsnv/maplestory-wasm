#!/bin/bash
# One-off host setup for maplestory-wasm dev.
# Adds the current user to the docker group and installs the local
# Emscripten + CMake toolchain so ./scripts/build_wasm.sh works natively.
# Run with: bash scripts/setup_dev_env.sh
set -e

AUR_HELPER=""
for h in paru yay; do
  if command -v "$h" >/dev/null 2>&1; then AUR_HELPER="$h"; break; fi
done
if [ -z "$AUR_HELPER" ]; then
  echo "No AUR helper (paru/yay) found. Install one first." >&2
  exit 1
fi
echo "Using AUR helper: $AUR_HELPER"

# 1. Docker group membership (lets containers run without sudo).
if ! id -nG | grep -qw docker; then
  echo "Adding $USER to the docker group..."
  sudo usermod -aG docker "$USER"
  echo "(Takes effect on next login / new shell. Newgrp below will apply it now.)"
else
  echo "Already in docker group."
fi

# 2. Local WASM toolchain.
echo "Installing emscripten and cmake via $AUR_HELPER..."
"$AUR_HELPER" -S --needed emscripten cmake

echo ""
echo "Setup done."
echo "  - Log out and back in (or run: newgrp docker) so the docker group applies."
echo "  - Then re-run this agent and it will build + launch the stack."
