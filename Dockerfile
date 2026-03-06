ARG UBUNTU_VERSION=24.04
FROM ubuntu:${UBUNTU_VERSION}
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ARG ZIG_VERSION=0.14.0
ARG CMAKE_VERSION=3.28.6
ARG TARGET_GLIBC=2.17
ARG USERNAME=app

# Install system packages. hadolint complains about pinning versions
# but these are effectivly pinned by the ubuntu version...so suppress
# the warning
# hadolint ignore=DL3008
RUN apt-get update \
  && apt-get install -y --no-install-recommends \
  ninja-build sudo bash coreutils ca-certificates curl xz-utils zip git \
  freeglut3-dev libudev-dev libopenal-dev \
  llvm-18 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

# Install CMake.
RUN curl -sSL https://cmake.org/files/v${CMAKE_VERSION%.*}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz \
  | tar -xz -C /usr/local/ --strip-components=1

# Install Zig.
RUN curl -sSL https://ziglang.org/download/${ZIG_VERSION}/zig-linux-x86_64-${ZIG_VERSION}.tar.xz \
  | tar -xJ -C /opt/ \
  && ln -s /opt/zig-linux-x86_64-${ZIG_VERSION} /opt/zig \
  && ln -s /opt/zig/zig /usr/local/bin/zig

# Create Zig wrapper scripts for cross-compilation.
RUN set -e; \
  for entry in \
    "zig-cc-linux|cc|x86_64-linux-gnu.${TARGET_GLIBC}" \
    "zig-cxx-linux|c++|x86_64-linux-gnu.${TARGET_GLIBC}" \
    "zig-cc-windows|cc|x86_64-windows-gnu" \
    "zig-cxx-windows|c++|x86_64-windows-gnu" \
    "zig-cc-macos-x64|cc|x86_64-macos" \
    "zig-cxx-macos-x64|c++|x86_64-macos" \
    "zig-cc-macos-arm64|cc|aarch64-macos" \
    "zig-cxx-macos-arm64|c++|aarch64-macos"; \
  do \
    IFS='|' read -r name mode target <<< "$entry"; \
    printf '#!/bin/sh\nexec zig %s -target %s "$@"\n' "$mode" "$target" \
      > /usr/local/bin/"$name"; \
    chmod +x /usr/local/bin/"$name"; \
  done

# Create user with passwordless sudo.
# Ubuntu 24.04 ships with a 'ubuntu' user at UID 1000; remove it first.
RUN userdel -r ubuntu 2>/dev/null || true \
  && useradd -m -u 1000 ${USERNAME} \
  && usermod -aG sudo ${USERNAME} \
  && echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

VOLUME /src
USER ${USERNAME}

WORKDIR /src
