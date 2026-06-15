# [LibreMaple](https://libremaple.org/) Client: a fork/continuation of JourneyClient

[![License](https://img.shields.io/badge/license-AGPL%20v3.0%2B-brightgreen.svg)](https://www.gnu.org/licenses/agpl-3.0.html) [![C++ standard](https://img.shields.io/badge/C%2B%2B-14%2F17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17) [![Discord](https://img.shields.io/badge/chat-Discord%20%7C%20%23libremaple__dev-brightgreen.svg)](https://discord.gg/xHUzzHT) [![clang++ version](https://img.shields.io/badge/clang%2B%2B-5.0-dd66aa.svg)](http://clang.llvm.org/) [![Maplestory version](https://img.shields.io/badge/Maplestory-v83-orange.svg)](https://en.wikipedia.org/wiki/MapleStory)

## JourneyClient

JourneyClient is a custom, made-from-scratch game client for
[Maplestory](https://en.wikipedia.org/wiki/MapleStory).
JourneyClient is written in [C++](https://en.wikipedia.org/wiki/C%2B%2B) and
was created by Daniel Allendorf in 2015.

The [LibreMaple](https://libremaple.org/) project aims to make a version of
Maplestory that is
[100% free and open](https://www.fsf.org/about/what-is-free-software) on both
the server and client sides, to preserve the freedom of its users and enable
creativity and collaboration around the game. As such, the LibreMaple project
takes the JourneyClient as a starting point.

LibreMaple aims to make the client
[cross-platform](https://en.wikipedia.org/wiki/Cross-platform)
([GNU+Linux](https://en.wikipedia.org/wiki/Linux)/[macOS](https://en.wikipedia.org/wiki/MacOS)/[Windows NT](https://en.wikipedia.org/wiki/Windows_NT)/[FreeBSD](https://en.wikipedia.org/wiki/FreeBSD)/etc.)
as well, so decisions are made with a cross-platform mindset as well as a free
software one.

## Supported versions

The client is currently compatible with version 83 servers. For the UI file
(UI.nx), one from version 154+ should be used.

## How to compile/install

### [Unix-like systems](https://en.wikipedia.org/wiki/Unix-like) (GNU+Linux, FreeBSD, etc.) (may also work for Unix-like Windows subsystems like [Cygwin](https://en.wikipedia.org/wiki/Cygwin), [MinGW](https://en.wikipedia.org/wiki/MinGW), or [WSL](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux))

#### Dependencies

* [clang](http://clang.llvm.org/) (version 5+)
* [cmake](https://cmake.org/) (version 3.8+)
* [make](https://www.gnu.org/software/make/) (GNU Make version 4+)
* [git](https://git-scm.com/)
* [wget](https://www.gnu.org/software/wget/)
* [tar](https://www.gnu.org/software/tar/)
* unzip
* [autoconf](https://www.gnu.org/software/autoconf/autoconf.html)
* [libtool](https://www.gnu.org/software/libtool/)
* [sh or bash](https://en.wikipedia.org/wiki/Bourne_shell)
* [glut](http://freeglut.sourceforge.net/) / freeglut3, freeglut3-dev (an implementation of GLUT, including development files)
* libXmu (including the "development" version, if that's separate)
* libXi (including the "development" version)
* libgl-dev / libgl1-mesa-dev / libgl / mesa / libGL-devel (implementation of OpenGL with development files)
* libosmesa-dev / libosmesa / osmesa (skip this if your package manager doesn't have it)
* xorg / xorg-dev (Xorg development libraries)
* build-essential (for users of Debian-based distibutions only)

(**Note for [Solus](https://solus-project.com/) users:**
`sudo eopkg it -c system.devel`)

Most of these dependencies should be available from your package manager
(`pacman`, `apt`, `dnf`, `zypper`, `pkg`, `xbps-install`, `eopkg`, `yum`,
etc.), but for all dependencies listed with a version number, **make sure
to check that the version your package manager gives you is high enough**.
If your package manager doesn't have something or doesn't have a high enough
version, you may have to install it using instructions from the relevant
website or use auxilliary package management (`yaourt`, PPAs, `copr`, etc.).

#### Instructions

```bash
$ git clone https://github.com/Libre-Maple/LibreMaple-Client.git
$ git clone https://github.com/NoLifeDev/NoLifeNx.git nlnx

# The next command is probably best replaced by navigating to
# https://sourceforge.net/projects/asio/files/latest/download
# and downloading from there to get the latest stable version of ASIO.
$ wget https://downloads.sourceforge.net/project/asio/asio/1.10.8%20%28Stable%29/asio-1.10.8.tar.bz2
$ mkdir asio
$ tar xf asio-* --strip-components=1 -C asio/
$ rm asio-*

$ git clone https://github.com/glfw/glfw.git

# Again, the following command can be replaced by just going to
# https://sourceforge.net/projects/glew/files/glew
# and downloading the latest version of GLEW.
$ wget https://downloads.sourceforge.net/project/glew/glew/2.1.0/glew-2.1.0.tgz
$ mkdir glew
$ tar xf glew-* --strip-components=1 -C glew/
$ rm glew-*

$ git clone git://git.sv.nongnu.org/freetype/freetype2.git freetype

# www.un4seen.com
$ wget http://www.un4seen.com/files/bass24-linux.zip
$ mkdir bass
$ unzip bass2* -d bass/
$ rm bass2*

$ git clone https://github.com/lz4/lz4.git

$ export CC='/usr/bin/clang'
$ export CXX='/usr/bin/clang++'

$ cd freetype
$ sh autogen.sh
$ ./configure
$ make
$ cd ..

$ cd glew
$ make
$ cd ..

$ cd glfw
$ mkdir build
$ cd build
$ cmake ..
$ make
$ cd ../..

$ cd lz4
$ make
$ cd ..

$ cd LibreMaple-Client
$ mkdir build
$ cd build
$ cmake ..
$ make
```

If all this is successful, you should have the executable in your current
directory (`LibreMaple-Client/build`).

## Testing

The client has a fast, host-native unit-test binary (no GL context, network, or
WZ assets required) built with [doctest](https://github.com/doctest/doctest)
(vendored at `includes/doctest/doctest.h`). It is gated behind the `BUILD_TESTS`
CMake option and is independent of the WASM/native client build.

From `src/client/`:

```sh
$ cmake -S . -B build-tests -DBUILD_TESTS=ON
$ cmake --build build-tests --target tests
$ (cd build-tests && ctest --output-on-failure)
```

Or use the helper scripts from the repo root, which wrap the three steps above
(extra arguments are forwarded to `ctest`, e.g. `-R Keyboard`):

```sh
$ ./scripts/run_tests.sh             # build + run on the host
$ ./scripts/docker_run_tests.sh      # build + run inside the Docker builder image
```

The Docker runner reuses the `wasm-builder` service (which now also carries a
native toolchain + FreeType) and uses a separate `build-tests-docker/` dir so
the Linux container's CMake cache never collides with a host build.

Tests live in `Test/` and cover three layers:

- **Pure logic** (`test_packet_io`, `test_cryptography`, `test_util`,
  `test_template`, `test_keyboard`): wire format, crypto round-trips, geometry,
  containers, and keyboard mapping. (`Keyboard` uses a small in-repo GLFW
  key-code shim, `Test/glfw_shim/`, so no system GLFW is required.)
- **Headless rendering** (`test_graphics_headless`): the target defines
  `MS_HEADLESS`, which compiles every GL call out of `GraphicsGL` while keeping
  FreeType glyph layout and all quad/atlas bookkeeping (see
  `Graphics/GLHeadless.h`). Tests drive the draw API and assert on the recorded
  render list via the test-only `GraphicsGL::peek_quads()` — no GPU, no context.
- **Synthetic assets + real components** (`test_nx_synthetic`,
  `test_texture_render`, `test_button_render`): `Test/NxBuilder` serializes a
  synthetic NX (PKG4) image in code which is loaded through the real `nl::nx`
  reader via `nl::file::open_memory()` (no committed binary fixture). Real
  `Texture` and `MapleButton` objects are then constructed from those assets and
  exercised against the headless render list, including hit-testing and state.

FreeType is the only external dependency (located via `pkg-config`; on macOS
`brew install freetype`). See `plans/07-ui-testing.md` for the full roadmap.
Full `UIElement` windows additionally pull in the `UI` singleton hub, so
window-level tests await the singleton decoupling in `plans/06-*`.

### Windows NT (Windows 7, 8, 10+)

Coming soon...

#### Dependencies

Coming soon...

#### Instructions

Coming soon...

## Compiler

LibreMaple is built against the latest version of
[Clang](http://clang.llvm.org/) (5.0.X as of this writing), again for
cross-platform considerations and efficiency. [The GCC](https://gcc.gnu.org/)
is another excellent choice, especially for those on Unix-like operating
systems, but it is not officially supported by LibreMaple.

That being said, don't be shy to file issues involving GCC or to make PRs to
improve ease of use/compatibility with it.

## C++ details

JourneyClient was originally developed under
[the C++14 standard](https://en.wikipedia.org/wiki/C%2B%2B14). However,
**LibreMaple strictly uses
[the C++17 standard](https://en.wikipedia.org/wiki/C%2B%2B17) and makes use of C++17 features**.

In addition, **this project follows
[the ISO C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines)**,
so any code that does not comply with said guidelines can be considered a bug,
unless of course the transgression has been justified and thoroughly commented.
[A complete copy of the guidelines is available here](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md).

## Configuration

The build can be configured by editing the Journey.h file.
The following options are available:
- `JOURNEY_USE_CRYPTO` : Use cryptography when communicating for the server.
- `JOURNEY_USE_XXHASH` : Use xxhash for file check (additional dependency)
- `JOURNEY_USE_ASIO` : Use Asio for networking (additional dependency)

LibreMaple uses crypto (`JOURNEY_USE_CRYPTO`), as well as ASIO
(`JOURNEY_USE_ASIO`) to maintain cross-platform compatibility.

## Dependencies

- Nx library:
[NoLifeNX](https://github.com/NoLifeDev/NoLifeNx) (depends on
[LZ4](https://github.com/lz4/lz4))

- Graphics:
[GLFW3](http://www.glfw.org/download.html),
[GLEW](http://glew.sourceforge.net/),
[FreeType](http://www.freetype.org/)

- Audio:
[Bass](http://www.un4seen.com/)

- Utility:
[xxHash](https://github.com/Cyan4973/xxHash) (optional)

- Networking:
[Asio](http://think-async.com/) (optional)
