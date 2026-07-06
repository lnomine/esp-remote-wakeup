FROM debian:trixie-slim AS builder

ARG DEPS="git python-is-python3 python3-venv libusb-1.0-0 cmake make"

WORKDIR /work

RUN apt-get update && \
    apt-get install --no-install-recommends -y $DEPS && \
    rm -rf /var/lib/apt/lists/*

COPY . .

RUN chmod +x 00-init.sh 00-set-target.sh && \
    ./00-init.sh && \
    ./00-set-target.sh esp32s3 && \
    find /work -type d -name ".git" -prune -exec rm -rf {} + && \
    tar -czf /work.tar.gz -C / work


FROM debian:trixie-slim

ARG DEPS="git python-is-python3 python3-venv libusb-1.0-0 cmake make"

WORKDIR /work

RUN apt-get update && \
    apt-get install --no-install-recommends -y $DEPS && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /work.tar.gz /work.tar.gz

RUN printf '%s\n' \
'#!/bin/sh' \
'set -e' \
'' \
'if [ ! -f /work/.initialized ]; then' \
'    echo "Extracting full /work..."' \
'    tar -xzf /work.tar.gz -C /' \
'    touch /work/.initialized' \
'fi' \
'' \
'exec "$@"' \
> /usr/local/bin/docker-entrypoint.sh && \
chmod +x /usr/local/bin/docker-entrypoint.sh

ENTRYPOINT ["/usr/local/bin/docker-entrypoint.sh"]
CMD ["bash"]
