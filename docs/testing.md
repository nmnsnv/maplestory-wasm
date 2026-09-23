# Client tests

The native tests use CMake, CTest and doctest. They exercise client code without
starting a browser, GPU context, web services or Cosmic. No live-server tests are
included. The WASM build remains a separate check because native tests cannot
validate Emscripten, WebGL or LazyFS browser behavior.

## Run locally

The default suite needs CMake 3.21+, a native C++17 compiler (Clang or GCC), and
a build tool such as Make. The first configure downloads doctest 2.4.11 and
verifies its SHA-256; subsequent runs reuse the dependency and compiled objects.
`CC` and `CXX` can select the compiler when configuring a new build directory.

```bash
./scripts/run_tests.sh
./scripts/run_tests.sh -- --output-on-failure -R packet
./scripts/run_tests.sh -- -N                    # List individual cases
./scripts/run_tests.sh --release               # Assertions stay enabled
./scripts/run_tests.sh --sanitize              # Address + undefined behavior checks
./scripts/run_tests.sh --jobs 4
```

Everything after `--` is passed to CTest. A failing test, failed build, or filter
matching no tests returns a nonzero status. Listing with `-N` does not run tests.
The runner sets UBSan to stop on the first error by default; `UBSAN_OPTIONS` can
override its diagnostics for investigation.

Some Apple Clang/OS combinations can stall inside ASan initialization before
test discovery. Use the Docker sanitizer command below, or select an installed
LLVM compiler with `CC`/`CXX` for a fresh sanitizer build directory.

## Tests using NX assets

```bash
./scripts/run_tests.sh --assets                 # Unit and asset suites
./scripts/run_tests.sh --assets -- -L assets    # Only asset cases
./scripts/run_tests.sh --assets -- -R npc       # NPC regressions
TEST_ASSET_DIR=/path/to/existing/nx ./scripts/run_tests.sh --assets
```

Asset tests also need FreeType headers/library and native OpenGL headers. On
macOS, the Xcode command-line tools supply OpenGL and `brew install freetype`
supplies FreeType. On Ubuntu, install `libfreetype-dev libgl-dev`.

The asset suite requires `Character.nx`, `Etc.nx`, `Item.nx`, `Npc.nx`, `Quest.nx`,
`String.nx`, `UI.nx`, and `UI_83.nx`. `UI.nx` is the client's newer UI set;
`UI_83.nx` supplies the classic Cash Shop artwork. Configuration fails with the
missing path when an explicitly enabled asset suite lacks a required file.
These are compatibility tests against the local asset set, so a different set
can produce legitimate differences. The default suite never opens these files.

All NX data is read-only. Tests create diagnostic images and text files in their
build directories, never under `assets/`.

## Docker fallback

When the native toolchain or optional asset dependencies are unavailable:

```bash
./scripts/docker_run_tests.sh
./scripts/docker_run_tests.sh --assets
./scripts/docker_run_tests.sh --sanitize
```

The fallback accepts the same arguments and `TEST_ASSET_DIR`. Its image includes
the verified doctest dependency. Image creation needs internet access; test
execution uses `--network none`. Existing assets are mounted read-only. It starts
no web services and uses no shared game-server network.

## Build directories and reports

Host builds live in `build/tests/native-Debug/`, with `-assets` and `-sanitized`
suffixes as requested; `--release` selects `native-Release`. Docker prefixes its
directories with `docker-` so compiler caches are never shared with the host.

Each build directory contains:

- `reports/junit.xml`: the CTest result, suitable for CI ingestion.
- `Testing/Temporary/LastTest.log`: complete test output, including failures.
- `work/<executable>/artifacts/`: PPM previews, TSV text placement, and raw BGRA
  reward artwork from the existing drawing regressions. These are diagnostic
  captures, not pixel-comparison baselines.

JUnit from a previous run is removed before configuration, so a failed build
cannot leave an old successful report behind. Tests run with a 120-second
timeout and are individually discoverable. Cases execute in separate processes
to isolate the client's existing singletons and metadata caches.

The `native-tests` GitHub Actions workflow runs the self-contained suite on Linux,
on macOS in Release, and on Linux with sanitizers. It uploads reports and failure
logs even after a test failure. Assets are not distributed in Git, so asset tests
remain an explicit local/Docker check. The existing WASM build workflow remains
enabled alongside native tests.

## Add a test

Write named `TEST_CASE`s in `tests/<feature>_test.cpp`, using doctest `CHECK`,
`REQUIRE`, `CHECK_THROWS_AS`, `SUBCASE`, and `CAPTURE` as appropriate. Use `REQUIRE`
for preconditions before indexing a container; use `CHECK` for independent
expectations. Do not use runtime `assert`, which can disappear in Release.

Register a new executable in `tests/CMakeLists.txt`:

```cmake
client_test(example_test unit)
# Inside the TEST_ASSETS block:
client_test(example_assets_test assets test_asset_client test_asset_support)
```

Cases added to an existing executable are discovered automatically. Add production
sources to the shared `test_core` or `test_asset_client` target when necessary;
they compile once per build variant. Keep separate executables for scenarios
whose legacy boundary stubs define conflicting production symbols.

Reusable support lives in `tests/support/`:

- `packet_fixture.h`: owned packet bytes and byte-by-byte wire-format checks.
- `assets.h`: explicit NX paths, scoped root binding, and artifact paths. Files
  stay open for the process lifetime because production metadata caches hold nodes.
- `inventory_stub.h`: controlled inventory contents for quest scenarios, reset
  between cases. Inventory behavior itself is tested with the production class.
- `render_capture.h`: captured bitmap draws and a shared PPM compositor supporting
  scaling, mirroring and opacity.
- `texture_capture.cpp`: a shared boundary stub for character/equipment composition
  tests. NPC button tests separately link production `Texture.cpp`.

Use client-observable results and independent protocol fixtures based on
[`ms-network-protocol.md`](ms-network-protocol.md). Control time through existing
client inputs (for example `Questlog::set_server_time`), not sleeps. Restore any
global state when adding cases to an executable that can also be run directly.
Do not add server provisioning, account creation, or database resets.

The per-feature `scripts/test_*.sh` launchers were replaced by this runner. The
obsolete status-bar test remains removed.
