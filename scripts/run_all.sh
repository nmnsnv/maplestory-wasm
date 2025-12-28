#!/bin/bash
set -e

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "---------------------------------------------------"
echo "Starting MapleStory WASM Stack (Server + Web + Proxy)"
echo "---------------------------------------------------"


# 0. Create Shared Network
echo "[0/2] Creating Docker Network..."
docker network create maplestory-network || true

# 1. Start Game Server
echo "[1/2] Starting Game Server (Java + DB)..."
./scripts/run_server.sh -d --remove-orphans

# 2. Start Web Server & Proxy
echo "[2/2] Starting Web Server & WS Proxy..."
./scripts/docker_web_up.sh -d --remove-orphans

echo "---------------------------------------------------"
echo "All services started successfully!"
echo ""
echo "Game Server Ports: 8484 (Login), 7575-7585 (Channel)"
echo "Web URL:           http://localhost:8000"
echo "WS Proxy URL:      ws://localhost:8080"
echo ""
echo "To view logs:"
echo "  Server: cd src/server && docker compose logs -f"
echo "  Web:    docker compose logs -f"
echo "---------------------------------------------------"
