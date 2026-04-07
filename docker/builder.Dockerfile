FROM emscripten/emsdk:4.0.21

WORKDIR /app

# Ensure git is available if needed by build scripts
RUN apt-get update && apt-get install -y git

# Default command to run the build script
CMD ["./scripts/build_wasm.sh"]
