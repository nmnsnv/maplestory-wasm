#!/bin/bash
set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "---------------------------------------------------"
echo "Stopping MapleStory WASM Web Stack"
echo "---------------------------------------------------"

# 1. Stop Web Services
echo "[1/2] Stopping Docker web services..."
docker compose down

# 2. Remove Shared Network
echo "[2/2] Removing Docker Network..."
docker network rm maplestory-network || true

echo "---------------------------------------------------"
echo "All services stopped successfully!"
echo "---------------------------------------------------"
