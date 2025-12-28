# WebSocket Proxy for MapleStory WASM Client

This directory contains the WebSocket-to-TCP proxy needed for the MapleStory WASM client to connect to regular MapleStory servers.

## Why is this needed?

Browsers cannot make direct TCP connections due to security restrictions. The WASM client uses WebSockets instead, which need to be bridged to the TCP protocol used by MapleStory servers.

## Running the Proxy

### Prerequisites

Install the `websockets` library:

```bash
pip install websockets
```

### Start the Proxy

```bash
python web/ws_proxy.py
```

By default, the proxy:
- Listens for WebSocket connections on `ws://0.0.0.0:8485`
- Connects to the MapleStory server at `127.0.0.1:8484`

### Custom Configuration

You can customize the ports and target server:

```bash
python web/ws_proxy.py --ws-port 8485 --tcp-host 127.0.0.1 --tcp-port 8484
```

## Complete Setup

To run the WASM client, you need to start both servers:

**Terminal 1 - HTTP Server (for WASM files):**
```bash
python web/server.py
```

**Terminal 2 - WebSocket Proxy (for networking):**
```bash
python web/ws_proxy.py
```

**Terminal 3 - MapleStory Server (if running locally):**
```bash
# Your MapleStory server startup command
```

Then open your browser to `http://localhost:8000`

## How it Works

```
Browser (WASM Client) <--WebSocket--> Proxy <--TCP--> MapleStory Server
     ws://localhost:8485                    127.0.0.1:8484
```

The proxy is a transparent bridge that:
1. Accepts WebSocket connections from the browser
2. Establishes a TCP connection to the MapleStory server
3. Forwards all data bidirectionally between the two connections
4. Handles connection lifecycle and cleanup
