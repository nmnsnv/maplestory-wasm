#!/bin/bash
set -e

# Compile nxdump
# -I../../includes/NoLifeNx: For nlnx headers
# -I../../deps/lz4: For lz4.h if needed
# Sources: nxdump.cpp, all nlnx cpp files, and lz4.c

echo "Building nxdump..."
clang++ -std=c++17 -O2 -o nxdump \
    nxdump.cpp \
    ../../includes/NoLifeNx/nlnx/*.cpp \
    ../../deps/lz4/lz4.c \
    -I../../includes/NoLifeNx \
    -I../../deps/lz4 \
    -framework AudioToolbox \
    -framework CoreFoundation

echo "Build complete: ./nxdump"
