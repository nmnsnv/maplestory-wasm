# 🍁 MapleStory WASM

> **A WebAssembly port of MapleStory v83, playable directly in your browser.**

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![Status: In Development](https://img.shields.io/badge/status-in%20development-orange.svg)]()
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen.svg)]()

MapleStory WASM is a preservation project that brings the classic MapleStory v83 experience to modern web browsers using WebAssembly. This project **patches and integrates** two separate open-source projects—a C++ client and a Java server—making them compatible and able to run together seamlessly.

---

## 🎯 Built On

This project is made possible by patching and extending the following upstream projects:

| Component | Upstream Project | Description |
|-----------|------------------|-------------|
| **Client** | [ryantpayton/MapleStory-Client](https://github.com/ryantpayton/MapleStory-Client) | A C++ MapleStory v83 client |
| **Server** | [ryantpayton/MapleStory-Server](https://github.com/ryantpayton/MapleStory-Server) | A Java MapleStory v83 server emulator |

Our patch system maintains compatibility patches on top of these repositories, enabling:
- **WebAssembly compilation** of the C++ client via Emscripten
- **Browser networking** through WebSocket proxies
- **Client-server compatibility** fixes and enhancements
- **Easy upstream updates** without maintaining forks

---

## ✨ Features

| Feature | Status |
|---------|--------|
| Character Creation & Login | ✅ Working |
| Map Rendering & Navigation | ✅ Working |
| Mob Spawning & Combat | ✅ Working |
| Multiplayer Sync | ✅ Working |
| Job Advancement | ⚠️ Partial |
| Skills & Abilities | ⚠️ Partial |
| NPCs & Quests | 🔧 In Progress |

### 🎮 Current Gameplay Status

> [!NOTE]
> **Explorers** are currently the only character type at an acceptable playability level.

| Character Type | Status | Notes |
|----------------|--------|-------|
| **Explorers** | ⚠️ Playable | The main playable class type |
| Cygnus Knights | ❌ Not Ready | Not yet implemented |
| Aran | ❌ Not Ready | Not yet implemented |
| Evan | ❌ Not Ready | Not yet implemented |

#### Explorer Job Advancement

| Job | 1st Job | Notes |
|-----|---------|-------|
| **Warrior** | ✅ Working | Job selection functional |
| **Magician** | ✅ Working | Job selection functional |
| **Bowman** | ✅ Working | Job selection functional |
| **Thief** | ❌ Broken | NPC flow is missing |
| **Pirate** | ❓ Untested | — |

### ⚠️ Known Limitations

> [!CAUTION]
> **Skill Assignment is currently broken.** Players cannot assign skill points to any job skills. This is the highest-priority issue being worked on.

| Limitation | Impact | Status |
|------------|--------|--------|
| **Cannot assign skill points** | Major - Blocks progression | 🔧 Being investigated |
| Advanced job advancements | Not fully tested | 📋 Planned |

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Browser (Client)                            │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │                  MapleStory WASM Client                     │    │
│  │                 (C++ compiled to WASM)                      │    │
│  └─────────────────────────────────────────────────────────────┘    │
└───────────────┬─────────────────────────────────┬───────────────────┘
                │ WebSocket                       │ WebSocket
                │ (Game Packets)                  │ (Asset Requests)
                ▼                                 ▼
┌───────────────────────────┐       ┌───────────────────────────┐
│     WS Proxy (Python)     │       │   Assets Server (Python)  │
│   ws://localhost:8080     │       │   ws://localhost:8765     │
└───────────────┬───────────┘       └───────────────────────────┘
                │ TCP
                ▼
┌───────────────────────────────────────────────────────────────┐
│                    MapleStory Server (Java)                   │
│              Login: 8484  │  Channels: 7575-7585              │
└───────────────────────────┬───────────────────────────────────┘
                            │
                            ▼
                    ┌───────────────┐
                    │    MySQL DB   │
                    └───────────────┘
```

### How It Works

1. **WASM Client** - The original C++ MapleStory client is compiled to WebAssembly using Emscripten, allowing it to run natively in browsers.
2. **WebSocket Proxy** - Browsers cannot make raw TCP connections, so a Python proxy bridges WebSocket connections to the Java game server.
3. **LazyFS** - A dynamic file system technology that streams game assets (`.nx` files) on-demand via WebSocket and caches them locally in your browser. Assets are only fetched from the network once, providing native loading times on subsequent loads.
4. **Patch System** - We maintain patches on top of upstream repositories, enabling easy updates without forking.

---

## ⚠️ Required Game Assets

> [!IMPORTANT]
> You **must** provide your own game assets to run this project. We cannot distribute them due to copyright.

### 1. Client Assets (`.nx` files)
Place the `.nx` files into the `assets/` directory at the project root.
- These are the same `.nx` files required by the [MapleStory-Client](https://github.com/ryantpayton/MapleStory-Client) repository.
- **Location:** `maplestory-wasm/assets/*.nx`

### 2. Server Assets (`.wz` files)
Place the original v83 `.wz` files into the `serverAssets/` directory at the project root.
- These are the same `.wz` files required by the [MapleStory-Server](https://github.com/ryantpayton/MapleStory-Server) repository.
- **Location:** `maplestory-wasm/serverAssets/*.wz`

---

## 🚀 Quick Start

### 🐳 Hosting with Docker (Recommended)

> **Only Docker is required to host the game!** No other tools needed.

| Requirement | Version | Notes |
|-------------|---------|-------|
| **Docker** | 20.10+ | Includes Docker Compose |

```bash
# Clone the repository
git clone https://github.com/nmnsnv/maplestory-wasm.git
cd maplestory-wasm

# Start everything (Server + Web + Proxy + Database)
./scripts/run_all.sh
```

Then open **http://<Your-LAN-IP>:8000** in your browser (e.g., `192.168.1.X:8000`).

> [!WARNING]
> You **must** use your LAN IP. Accessing via `localhost` or `127.0.0.1` **will not work** due to docker networking.

> **Note:** The Docker setup automatically syncs sources, applies patches, builds the WASM client, and starts all services.

---

### 🛠️ Development Setup (Manual)

<details>
<summary>Click to expand manual setup for development</summary>

If you want to develop or build locally without Docker, you'll need additional tools:

| Requirement | Version | Purpose |
|-------------|---------|---------|
| **Python** | 3.9+ | Patch system scripts |
| **Java JDK** | 8 | Server compilation |
| **Emscripten** | 3.1+ | WASM compilation |
| **CMake** | 3.16+ | Build system |
| **MySQL** | 8.0+ | Database |

#### 1. Sync & Patch Sources

```bash
# Sync upstream client and server repositories
python3 patch_system/scripts/sync.py

# Apply our patches
python3 patch_system/scripts/apply_patches.py
```

#### 2. Build the WASM Client

```bash
# Ensure Emscripten is activated
source /path/to/emsdk/emsdk_env.sh

# Build the client
./scripts/build_wasm.sh

# For debug builds with symbols:
./scripts/build_wasm.sh --debug
```

#### 3. Configure & Start the Server

```bash
cd src/server

# Copy and edit configuration
cp configuration.ini.example configuration.ini
# Edit configuration.ini with your MySQL credentials

# Build and run
./launch.sh
```

#### 4. Start Web Services

```bash
# Terminal 1: HTTP Server
python web/server.py

# Terminal 2: WebSocket Proxy
python web/ws_proxy.py

# Terminal 3: Asset Server
python web/assets_server.py
```

#### 5. Play

Open **http://<Your-LAN-IP>:8000** in a modern browser. Do NOT use `localhost`.

</details>

---

## 📂 Project Structure

```
maplestory-wasm/
├── 📁 build/              # WASM build output
├── 📁 docker/             # Dockerfiles for services
├── 📁 patch_system/       # Patch management system
│   ├── deps.lock.json     # Upstream repo pinning
│   ├── patches/           # Patch files
│   └── scripts/           # Sync/apply/update scripts
├── 📁 scripts/            # Build & run scripts
├── 📁 src/                # Source code (gitignored)
│   ├── client/            # C++ MapleStory Client
│   └── server/            # Java MapleStory Server
├── 📁 web/                # Web infrastructure
│   ├── server.py          # HTTP server
│   ├── ws_proxy.py        # WebSocket-TCP proxy
│   └── assets_server.py   # NX asset streaming
├── 📄 docker-compose.yml  # Docker orchestration
├── 📄 LICENSE             # AGPL-3.0 License
└── 📄 README.md           # You are here
```

> **Note:** The `src/` directory is gitignored. It is populated by the patch system from upstream repositories.

---

## ⚙️ Configuration

### Server Configuration

Edit `src/server/configuration.ini`:

```ini
# Database
DB_HOST=localhost
DB_USER=maple
DB_PASS=yourpassword

# Network (use 0.0.0.0 for Docker)
HOST=0.0.0.0
LOGIN_PORT=8484
CHANNEL_PORT=7575
```

### Docker Environment

The `docker-compose.yml` provides sensible defaults. Key environment variables:

| Variable | Default | Description |
|----------|---------|-------------|
| `IS_DOCKER` | `true` | Enables Docker-specific networking |
| `DB_HOST` | `db` | MySQL container hostname |

---

## 🔧 Development

### Making Changes

1. **Edit files** in `src/client/` or `src/server/`
2. **Stage your changes** (for new files): `git add <file>`
3. **Save your patches**:
   ```bash
   python3 patch_system/scripts/update_patches.py
   ```
4. **Commit** the patch files in `patch_system/patches/`

### Updating Upstream

1. Edit `patch_system/deps.lock.json` with the new commit SHA
2. Re-sync: `python3 patch_system/scripts/sync.py`
3. Re-apply patches: `python3 patch_system/scripts/apply_patches.py`
4. Resolve any conflicts and update patches

### Build Commands

| Command | Description |
|---------|-------------|
| `./scripts/build_wasm.sh` | Build WASM client (Release) |
| `./scripts/build_wasm.sh --debug` | Build WASM client with debug symbols |
| `./scripts/build_wasm.sh -j 4` | Build with 4 parallel jobs |
| `./scripts/run_all.sh` | Start all services |
| `./scripts/stop_all.sh` | Stop all services |

---

## 🤝 Contributing

Contributions are welcome! Please read the guidelines below before submitting.

### How to Contribute

1. **Fork** the repository
2. **Create a branch** for your feature: `git checkout -b feature/amazing-feature`
3. **Make changes** and update patches (see [Making Changes](#making-changes))
4. **Test** your changes locally
5. **Commit** your changes: `git commit -m 'Add amazing feature'`
6. **Push** to your fork: `git push origin feature/amazing-feature`
7. **Open a Pull Request**

### Code Guidelines

- Follow the existing code style in each project (C++ client, Java server, Python scripts)
- Keep patches focused and well-documented
- Test across multiple browsers when modifying client code
- Update documentation for user-facing changes

### Areas for Contribution

- 🐛 Bug fixes and stability improvements
- 🎮 Missing game features (skills, NPCs, quests)
- 📖 Documentation improvements
- 🧪 Testing and QA
- 🎨 UI/UX enhancements

---

## 🙏 Acknowledgments

This project would not be possible without the incredible work of:

### Core Dependencies

| Project | Author | Contribution |
|---------|--------|--------------|
| [MapleStory-Client](https://github.com/ryantpayton/MapleStory-Client) | [@ryantpayton](https://github.com/ryantpayton) | The C++ client that we patch and compile to WASM |
| [MapleStory-Server](https://github.com/ryantpayton/MapleStory-Server) | [@ryantpayton](https://github.com/ryantpayton) | The Java server that we patch for compatibility |

### Inspiration & Tools

- **[HeavenClient/HeavenClient](https://github.com/HeavenClient/HeavenClient)** - Foundational work that inspired the client
- **[Emscripten](https://emscripten.org/)** - The toolchain making C++ in the browser possible
- **The MapleStory Community** - For keeping the nostalgia alive after all these years

**Special thanks to [@ryantpayton](https://github.com/ryantpayton)** for maintaining the client and server projects that form the foundation of this work.

---

## ⚠️ Disclaimer

This project is for **educational and preservation purposes only**.

- **MapleStory** is a trademark of **NEXON Korea Corporation**.
- All game assets, art, music, and related content are copyright of their respective owners.
- This project does not distribute any copyrighted game assets.
- Users must provide their own legal copies of game assets (`.nx` or `.wz` files).
- This project is not affiliated with, endorsed by, or connected to NEXON in any way.

**Use responsibly and respect intellectual property rights.**

---

## 📜 License

This project is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.

This means:
- ✅ You can use, modify, and distribute this software
- ✅ You can use it for commercial purposes
- ⚠️ You **must** disclose your source code if you deploy a modified version
- ⚠️ You **must** license your modifications under AGPL-3.0
- ⚠️ You **must** provide access to source code for network users

See the [LICENSE](LICENSE) file for full details.

---

<div align="center">

**Happy Mapling! 🍄**

*If this project brings back memories, consider giving it a ⭐*

</div>