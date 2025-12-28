#!/bin/bash
cd "$(dirname "$0")/.."

if ! docker network ls | grep -q "maplestory-network"; then
  echo "Creating network 'maplestory-network'..."
  docker network create maplestory-network
else
  echo "Network 'maplestory-network' already exists."
fi
