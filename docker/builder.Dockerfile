FROM emscripten/emsdk:4.0.21

WORKDIR /app

# Ensure git is available if needed by build scripts, plus the native toolchain
# and FreeType used by the host unit tests (./scripts/run_tests.sh).
#
# Disabling apt HTTP pipelining/caching works around the "invalid signature" apt
# error seen on Docker Desktop for Apple Silicon, where pipelined downloads in the
# build VM corrupt the fetched package index. Clearing stale lists + retries adds
# further resilience.
RUN printf 'Acquire::http::Pipeline-Depth "0";\nAcquire::http::No-Cache "true";\nAcquire::BrokenProxy "true";\n' \
        > /etc/apt/apt.conf.d/99fixbadproxy \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean \
    && apt-get update -o Acquire::Retries=5 \
    && apt-get install -y --no-install-recommends \
        git \
        build-essential \
        cmake \
        pkg-config \
        libfreetype6-dev \
    && rm -rf /var/lib/apt/lists/*

# Default command to run the build script
CMD ["./scripts/build_wasm.sh"]
