FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential clang libclang-rt-18-dev cmake pkg-config libfreetype-dev libgl-dev \
    ca-certificates curl \
    && rm -rf /var/lib/apt/lists/*

# Cache the verified test dependency in the image so runs need no network.
RUN curl --fail --location --silent --show-error \
      https://codeload.github.com/doctest/doctest/tar.gz/refs/tags/v2.4.11 -o /tmp/doctest.tar.gz \
    && echo '632ed2c05a7f53fa961381497bf8069093f0d6628c5f26286161fbd32a560186  /tmp/doctest.tar.gz' | sha256sum -c - \
    && mkdir -p /opt/doctest \
    && tar -xzf /tmp/doctest.tar.gz --strip-components=1 -C /opt/doctest \
    && rm /tmp/doctest.tar.gz

ENV CC=clang CXX=clang++
WORKDIR /app
CMD ["./scripts/run_tests.sh"]
