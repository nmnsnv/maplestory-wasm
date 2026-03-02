# AGENTS.md

## Scope

This file tells agents how to build and run this repository without guessing.
Prefer the local workflow first. Use Docker when the local workflow is not possible or the required local toolchain is unavailable.

## Hard Rules

- Never modify anything under `assets/`.
- Treat `assets/` as read-only input data at all times.
- Do not delete, move, rename, regenerate, or reformat files in `assets/`.
- Do not change build commands just to work around a missing local dependency. Use the documented Docker fallback instead.

## Repo Facts

- WASM client source: `src/client`
- Shared NX library: `src/nlnx`
- Client build output: `build/JourneyClient.js`, `build/JourneyClient.wasm`, and optionally `build/JourneyClient.wasm.map`
- Local web entrypoints: `web/server.py`, `web/ws_proxy.py`, `web/assets_server.py`
- Docker web stack: root `docker-compose.yml`
- Full Docker server startup also expects `src/server/` to exist

## Client Build

Preferred build command:

```bash
./scripts/build_wasm.sh
```

Useful variants:

```bash
./scripts/build_wasm.sh --debug # --debug is the same as -g
./scripts/build_wasm.sh --jobs 4
```

If the local Emscripten or CMake toolchain is unavailable, or the local build is not possible, use the Docker fallback:

```bash
./scripts/docker_build_wasm.sh
```

The Docker fallback accepts the same flags:

```bash
./scripts/docker_build_wasm.sh --debug
./scripts/docker_build_wasm.sh --jobs 4
```

## Local Deployment

Use local deployment when the local toolchain and local services are available.

1. Build the client with `./scripts/build_wasm.sh`.
2. If that is not possible, build the client with `./scripts/docker_build_wasm.sh`.
3. Install the local Python dependency if needed:

```bash
pip install -r web/requirements.txt
```

4. Start the local game server separately. The websocket proxy expects the game server to be reachable on TCP port `8484`.
5. Start the web services from the repository root in separate terminals:

```bash
python3 web/server.py
python3 web/ws_proxy.py --ws-port 8080
python3 web/assets_server.py --port 8765 --directory .
```

6. Open `http://localhost:8000`.

If `src/server/` is missing in the workspace, do not try to recreate it manually. Local full-stack deployment is unavailable until that source tree is present.

## Docker Deployment

Use Docker when local deployment is not practical.

Preferred full stack command:

```bash
./scripts/run_all.sh
```

This script creates the shared Docker network, starts the root web containers, and then starts the server stack from `src/server/`.

For the web side only:

```bash
./scripts/docker_web_up.sh -d
```

To stop everything:

```bash
./scripts/stop_all.sh
```

If `src/server/` is missing, `./scripts/run_all.sh` and `./scripts/run_server.sh` cannot start the game server. In that case, only use the client build and web-serving pieces, or restore the missing server checkout first.

## Agent Behavior

- Prefer `./scripts/build_wasm.sh` for client builds.
- Fall back to `./scripts/docker_build_wasm.sh` only when the local build is not possible.
- Prefer local deployment first when the environment supports it.
- Fall back to Docker deployment when local deployment is blocked by missing dependencies or services.
- Leave `assets/` untouched.

## Coding Styles:

* Write generic code whenever possible.
* Never write unsafe code.
* Add meaningful comments
  * Don't just repeat the code in the comment, explain the reasoning behind it.


## Task Completion
* Make sure the project builds.
