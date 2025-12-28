#!/bin/bash
set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "---------------------------------------------------"
echo "Stopping MapleStory WASM Stack"
echo "---------------------------------------------------"

# 1. Stop Web Server & Proxy
echo "[1/2] Stopping Web Server & WS Proxy..."
docker compose down

# 2. Stop Game Server
echo "[2/2] Stopping Game Server (Java + DB)..."
# Check if src/server directory exists and has a docker-compose.yml
if [ -d "src/server" ] && [ -f "src/server/docker-compose.yml" ]; then
    (cd src/server && docker compose down)
else
    echo "Warning: src/server/docker-compose.yml not found. Skipping server shutdown."
fi

# 3. Remove Shared Network
echo "[3/3] Removing Docker Network..."
docker network rm maplestory-network || true

echo "---------------------------------------------------"
echo "All services stopped successfully!"
echo "---------------------------------------------------"
