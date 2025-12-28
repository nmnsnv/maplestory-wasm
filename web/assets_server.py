#!/usr/bin/env python3
"""
WebSocket Asset Server for MapleStory WASM Client

A high-performance WebSocket server for streaming NX file chunks to the browser client.
Supports batched chunk requests with binary frames for efficient transfer.

Usage:
    python assets_server.py [--port 8765] [--directory .]

Protocol:
    Client → Server: {"type": "get_size", "file": "UI.nx"}
    Server → Client: {"type": "size", "file": "UI.nx", "size": 123456789}

    Client → Server: {"type": "get_chunks", "file": "UI.nx", "start": 0, "end": 10, "chunk_size": 524288}
    Server → Client: [binary: 4-byte chunk index + 1-byte filename len + filename + raw data]
    Server → Client: {"type": "chunks_done", "file": "UI.nx", "start": 0, "end": 10}
"""

import asyncio
import json
import os
import argparse
from pathlib import Path

try:
    import websockets
except ImportError:
    print("Please install websockets: pip install websockets")
    exit(1)

# Configuration
DEFAULT_PORT = 8765
DEFAULT_DIRECTORY = "."

# Global state
file_cache = {}  # Cache file handles for performance


class AssetServer:
    def __init__(self, directory: str):
        self.directory = Path(directory).resolve()
        self.file_sizes = {}  # Cache file sizes
        print(f"[AssetServer] Serving files from: {self.directory}")

    def get_file_path(self, filename: str) -> Path:
        """Get full path for a file, searching in common subdirectories."""
        # Prevent directory traversal by only using the name
        safe_name = Path(filename).name
        
        # Search locations in order
        candidates = [
            self.directory / safe_name,
            self.directory / "assets" / safe_name,
            self.directory / "serverAssets" / safe_name,
            self.directory / "wz" / safe_name,
            self.directory / "data" / safe_name,
        ]

        for path in candidates:
            if path.exists():
                return path

        # Default to root if not found (will be checked for existence later)
        return self.directory / safe_name

    def get_file_size(self, filename: str) -> int:
        """Get the size of a file."""
        if filename in self.file_sizes:
            return self.file_sizes[filename]
        
        filepath = self.get_file_path(filename)
        if not filepath.exists():
            print(f"[AssetServer] File not found: {filename} (checked {filepath} and subdirs)")
            return -1
        
        size = filepath.stat().st_size
        self.file_sizes[filename] = size
        return size

    def read_chunk(self, filename: str, chunk_index: int, chunk_size: int) -> bytes:
        """Read a specific chunk from a file."""
        filepath = self.get_file_path(filename)
        if not filepath.exists():
            return b""
        
        file_size = self.get_file_size(filename)
        start = chunk_index * chunk_size
        end = min(start + chunk_size, file_size)
        
        if start >= file_size:
            return b""
        
        with open(filepath, "rb") as f:
            f.seek(start)
            return f.read(end - start)

    async def handle_message(self, websocket, message: str):
        """Handle an incoming WebSocket message."""
        try:
            data = json.loads(message)
            msg_type = data.get("type")

            if msg_type == "get_size":
                filename = data.get("file")
                size = self.get_file_size(filename)
                await websocket.send(json.dumps({
                    "type": "size",
                    "file": filename,
                    "size": size
                }))

            elif msg_type == "get_chunks":
                filename = data.get("file")
                start_chunk = data.get("start", 0)
                end_chunk = data.get("end", start_chunk)
                chunk_size = data.get("chunk_size", 512 * 1024)

                # Send all requested chunks as BINARY frames
                # Format: [4 bytes: chunk index LE] [1 byte: filename len] [filename] [data]
                filename_bytes = filename.encode("utf-8")
                
                for chunk_idx in range(start_chunk, end_chunk + 1):
                    chunk_data = self.read_chunk(filename, chunk_idx, chunk_size)
                    if chunk_data:
                        # Build binary frame
                        header = (
                            chunk_idx.to_bytes(4, "little") +
                            len(filename_bytes).to_bytes(1, "little") +
                            filename_bytes
                        )
                        await websocket.send(header + chunk_data)
                
                # Send completion marker (text frame)
                await websocket.send(json.dumps({
                    "type": "chunks_done",
                    "file": filename,
                    "start": start_chunk,
                    "end": end_chunk
                }))

            elif msg_type == "get_chunk":
                # Single chunk request
                filename = data.get("file")
                chunk_idx = data.get("index", 0)
                chunk_size = data.get("chunk_size", 512 * 1024)
                
                chunk_data = self.read_chunk(filename, chunk_idx, chunk_size)
                if chunk_data:
                    # Send as binary frame
                    filename_bytes = filename.encode("utf-8")
                    header = (
                        chunk_idx.to_bytes(4, "little") +
                        len(filename_bytes).to_bytes(1, "little") +
                        filename_bytes
                    )
                    await websocket.send(header + chunk_data)
                else:
                    # Send error as text
                    await websocket.send(json.dumps({
                        "type": "error",
                        "message": f"Chunk not found: {filename}:{chunk_idx}"
                    }))

            else:
                await websocket.send(json.dumps({
                    "type": "error",
                    "message": f"Unknown message type: {msg_type}"
                }))

        except json.JSONDecodeError:
            await websocket.send(json.dumps({
                "type": "error",
                "message": "Invalid JSON"
            }))
        except Exception as e:
            print(f"[AssetServer] Error handling message: {e}")
            await websocket.send(json.dumps({
                "type": "error",
                "message": str(e)
            }))

    async def handler(self, websocket):
        """Handle a WebSocket connection."""
        remote = websocket.remote_address
        print(f"[AssetServer] Client connected: {remote}")
        
        try:
            async for message in websocket:
                await self.handle_message(websocket, message)
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            print(f"[AssetServer] Client disconnected: {remote}")


async def main(port: int, directory: str):
    server = AssetServer(directory)
    
    print(f"[AssetServer] Starting WebSocket server on port {port}")
    print(f"[AssetServer] Connect with: ws://localhost:{port}")
    
    async with websockets.serve(
        server.handler,
        "0.0.0.0",
        port,
        max_size=50 * 1024 * 1024,  # 50MB max message size
        compression=None  # Disable compression for speed
    ):
        await asyncio.Future()  # Run forever


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="WebSocket Asset Server")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"Port to listen on (default: {DEFAULT_PORT})")
    parser.add_argument("--directory", type=str, default=DEFAULT_DIRECTORY, help=f"Directory to serve files from (default: {DEFAULT_DIRECTORY})")
    args = parser.parse_args()

    try:
        asyncio.run(main(args.port, args.directory))
    except KeyboardInterrupt:
        print("\n[AssetServer] Shutting down...")
