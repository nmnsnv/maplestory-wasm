FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        git ca-certificates build-essential pkg-config liblz4-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/wz-converter
COPY container-convert.sh /opt/wz-converter/container-convert.sh
RUN chmod +x /opt/wz-converter/container-convert.sh

ENTRYPOINT ["/opt/wz-converter/container-convert.sh"]
