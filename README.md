# MapleStory WebAssembly Project

This repository contains the source code for running MapleStory in the browser using WebAssembly.

## 🐳 Running with Docker

You can run the entire stack (Game Server, Web Server, WebSocket Proxy, and Client Builder) using Docker.

### Prerequisites

*   **Docker** and **Docker Compose** installed on your machine.

---

### Step 1: Build the Client (WASM)

First, you need to compile the C++ client into WebAssembly. We provide a Docker container to handle the build environment (Emscripten).

Run the following command in the root directory:

```bash
./scripts/docker_build_wasm.sh
```

**What this does:**
1.  Builds the `wasm-builder` docker image.
2.  Runs the build container which mounts your source code.
3.  Compiles the `src/client` code into `web/MapleStory.js` and `web/MapleStory.wasm`.

> **Note:** This might take a few minutes for the first build.

---

### Step 2: Start Everything

You can start the Game Server, Web Server, and WebSocket Proxy with a single script.

Run the following command from the root directory:

```bash
./scripts/run_all.sh
```

**What this does:**
1.  Starts the `server-db` and `server-maplestory` containers in the background.
2.  Starts the `html-server` and `ws-proxy` containers in the background.
3.  Exposes all necessary ports for the game to function.

> **Viewing Logs:**
> *   **Game Server:** `cd src/server && docker compose logs -f`
> *   **Web/Proxy:** `docker compose logs -f`

### Stopping Everything

To stop all running services:

```bash
./scripts/stop_all.sh
```


---

### Step 3: Play!

1.  Open Chrome (or a compatible browser).
2.  Navigate to **[http://localhost:8000](http://localhost:8000)**.
3.  The game should load, downloading the WASM file and Assets.
4.  You should be able to connect to the server and play!

---

## 🛠 Troubleshooting

*   **"Range Not Satisfiable" / Asset Loading Errors:**
    *   Ensure your `serverAssets` (or equivalent wdb/nx files) are correctly placed where `docker/web.Dockerfile` or `server.py` expects them. By default, the web server serves files from the project root.
*   **Connection Failed:**
    *   Check if `ws-proxy` is running.
    *   Check if `server-maplestory` is running and healthy.
    *   The client defaults to connecting to `localhost`. If running on a remote server, update the IP configuration.
*   **Rebuilding:**
    *   If you change C++ code: Run `./scripts/docker_build_wasm.sh` again.
    *   If you change Java code: Restart the server with `./scripts/run_server.sh` (you may need `docker compose build` inside `src/server` if dependencies changed, but `run_server.sh` usually handles up).
