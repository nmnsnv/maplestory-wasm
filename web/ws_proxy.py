#!/usr/bin/env python3
"""
WebSocket-to-TCP Proxy for MapleStory WASM Client

This proxy bridges WebSocket connections from the browser to TCP connections
to the actual MapleStory server. It's necessary because browsers cannot make
direct TCP connections.

Usage:
    python ws_proxy.py [--ws-port WS_PORT] [--tcp-host TCP_HOST] [--tcp-port TCP_PORT]

Default configuration:
    - WebSocket listens on: ws://0.0.0.0:8080
    - TCP connects to: 127.0.0.1:8484
"""

import asyncio
import websockets
import argparse
import sys
import os


class MapleStoryProxy:
    """Proxy that forwards data between WebSocket and TCP connections."""

    async def handle_client(self, websocket):
        """Handle a WebSocket client connection."""
        client_addr = websocket.remote_address
        print(f"[WebSocket] Client connected from {client_addr}")

        tcp_reader = None
        tcp_writer = None

        try:
            # Read first message to get target TCP server address
            print(f"[WebSocket] Waiting for target address...")
            target_msg = await websocket.recv()
            
            if isinstance(target_msg, bytes):
                target_str = target_msg.decode('utf-8')
            else:
                target_str = target_msg
            
            # Parse target address (format: "host:port")
            try:
                tcp_host, tcp_port = target_str.strip().split(':')
                tcp_port = int(tcp_port)

                # If target is localhost/127.0.0.1, and we are in docker network, redirect to the game server container
                # The client (browser) sees 'localhost' as the host machine, but inside the container 'localhost' is itself.
                # Since we likely want to talk to the sibling 'server-maplestory' container:
                is_docker = os.environ.get("IS_DOCKER", "false").lower() == "true"
                if is_docker and tcp_host in ["localhost", "127.0.0.1", "0.0.0.0"]:
                    print(f"[Proxy] Redirecting {tcp_host} -> server-maplestory (Docker Internal Combined Network)")
                    tcp_host = "server-maplestory"

            except ValueError:
                print(f"[Error] Invalid target format '{target_str}', expected 'host:port'")
                await websocket.close()
                return
            
            print(f"[WebSocket] Client requested connection to {tcp_host}:{tcp_port}")

            # Connect to target TCP server
            print(f"[TCP] Connecting to {tcp_host}:{tcp_port}")
            tcp_reader, tcp_writer = await asyncio.open_connection(tcp_host, tcp_port)
            print(f"[TCP] Connected to target server")

            # Create tasks for bidirectional forwarding
            ws_to_tcp_task = asyncio.create_task(
                self.forward_ws_to_tcp(websocket, tcp_writer)
            )
            tcp_to_ws_task = asyncio.create_task(
                self.forward_tcp_to_ws(tcp_reader, websocket)
            )

            # Wait for either direction to complete (usually means disconnect)
            done, pending = await asyncio.wait(
                [ws_to_tcp_task, tcp_to_ws_task],
                return_when=asyncio.FIRST_COMPLETED
            )

            # Cancel pending tasks
            for task in pending:
                task.cancel()
                try:
                    await task
                except asyncio.CancelledError:
                    pass

        except ConnectionRefusedError:
            print(f"[TCP] Connection refused to {tcp_host}:{tcp_port}")
            print("[TCP] Is the target server running?")
        except Exception as e:
            print(f"[Error] Exception in client handler: {e}")
        finally:
            # Clean up connections
            if tcp_writer:
                tcp_writer.close()
                await tcp_writer.wait_closed()
            print(f"[WebSocket] Client {client_addr} disconnected")

    async def forward_ws_to_tcp(self, websocket, tcp_writer):
        """Forward data from WebSocket to TCP."""
        try:
            async for message in websocket:
                if isinstance(message, bytes):
                    # Binary message - forward to TCP
                    tcp_writer.write(message)
                    await tcp_writer.drain()
                    print(f"[WS→TCP] Forwarded {len(message)} bytes")
                else:
                    print(f"[WS→TCP] Warning: Received text message, expected binary")
        except websockets.exceptions.ConnectionClosed:
            print("[WS→TCP] WebSocket connection closed")
        except Exception as e:
            print(f"[WS→TCP] Error: {e}")

    async def forward_tcp_to_ws(self, tcp_reader, websocket):
        """Forward data from TCP to WebSocket."""
        try:
            while True:
                # Read data from TCP server
                data = await tcp_reader.read(8192 * 8)
                if not data:
                    print("[TCP→WS] TCP connection closed by server")
                    break

                # Send to WebSocket as binary
                await websocket.send(data)
                print(f"[TCP→WS] Forwarded {len(data)} bytes")

        except websockets.exceptions.ConnectionClosed:
            print("[TCP→WS] WebSocket connection closed")
        except Exception as e:
            print(f"[TCP→WS] Error: {e}")


async def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="WebSocket-to-TCP proxy for MapleStory WASM client"
    )
    parser.add_argument(
        "--ws-port",
        type=int,
        default=8080,
        help="WebSocket port to listen on (default: 8080)"
    )

    args = parser.parse_args()

    # Create proxy instance
    proxy = MapleStoryProxy()

    # Start WebSocket server
    print("=" * 60)
    print("Generic WebSocket-to-TCP Proxy")
    print("=" * 60)
    print(f"WebSocket server: ws://0.0.0.0:{args.ws_port}")
    print("Clients will specify target TCP server on connection")
    print("=" * 60)
    print("Waiting for connections...")

    async with websockets.serve(proxy.handle_client, "0.0.0.0", args.ws_port):
        await asyncio.Future()  # Run forever


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[Server] Shutting down...")
        sys.exit(0)
