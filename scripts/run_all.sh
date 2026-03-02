#!/bin/bash
set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "---------------------------------------------------"
echo "Starting MapleStory WASM Web Stack"
echo "---------------------------------------------------"


# 1. Create Shared Network
echo "[1/2] Creating Docker Network..."
docker network create maplestory-network || true

# 2. Start Web Server & Proxy
echo "[2/2] Starting Web Server, WS Proxy, and Asset Server..."
./scripts/docker_web_up.sh -d --remove-orphans

echo "---------------------------------------------------"
echo "All services started successfully!"
echo ""
echo "Web URL:           http://localhost:8000"
echo "WS Proxy URL:      ws://localhost:8080"
echo "Assets URL:        ws://localhost:8765"
echo ""
echo "To view logs:"
echo "  docker compose logs -f"
echo "---------------------------------------------------"
