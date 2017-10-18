# LibreMaple Client: a fork/continuation of JourneyClient

## JourneyClient

JourneyClient is a custom, made-from-scratch game client for Maplestory.
JourneyClient is written in C++ and was created by Daniel Allendorf in 2015.

The LibreMaple project aims to make a version of Maplestory that is 100% free
and open on both the server and client sides, to preserve the freedom of its
users and enable creativity and collaboration around the game. As such, the
LibreMaple project takes the JourneyClient as a starting point.

LibreMaple aims to make the client cross-platform (Linux/macOS/Windows/etc.) as
well, so decisions are made with a cross-platform mindset as well as a free
software one.

## Supported versions

The client is currently compatible with version 83 servers. For the UI file, a
v154+ version should be used.

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
[NoLifeNX](https://github.com/NoLifeDev/NoLifeNx)

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

## Compiler

LibreMaple is built against the latest version of Clang (currently 5.0.X),
again for cross-platform considerations and efficiency. The GCC is another
excellent choice, especially for those on Unix-like operating systems, but it
is not officially supported by LibreMaple.

That being said, don't be shy to file issues involving GCC or to make PRs to
improve ease of use/compatibility with it.
