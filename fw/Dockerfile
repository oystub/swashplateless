# syntax=docker/dockerfile:1.4

FROM ubuntu:24.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y sudo curl git patch python3 python3-venv && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

USER ubuntu
RUN mkdir -p /home/ubuntu/ws

# Copy sources
COPY --chown=ubuntu:ubuntu moteus /home/ubuntu/ws/moteus
COPY --chown=ubuntu:ubuntu mjlib  /home/ubuntu/ws/mjlib

# Use a cache mount for ~/.cache/bazel so incremental rebuilds are fast
RUN mkdir -p /home/ubuntu/artifacts
WORKDIR /home/ubuntu/ws/moteus
RUN --mount=type=cache,target=/home/ubuntu/.cache/bazel,uid=1000,gid=1000 \
    ./tools/bazel build \
      --workspace_status_command=/bin/true \
      --config=target \
      //:target && \
    cp bazel-bin/fw/moteus.elf /home/ubuntu/artifacts/ && \
    cp bazel-bin/fw/can_bootloader.elf /home/ubuntu/artifacts/

FROM scratch as export
COPY --from=builder /home/ubuntu/artifacts/moteus.elf /
COPY --from=builder /home/ubuntu/artifacts/can_bootloader.elf /
