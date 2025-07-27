# Docker-Based LibreOffice Build Environment

## Overview

This guide shows how to build LibreOffice from this core repository using Docker. This approach provides a consistent build environment across all platforms and eliminates dependency issues.

## Quick Start

### 1. Basic Docker Build

From the root of this repository (core-master):

```bash
# Build using Ubuntu in Docker (from this directory)
docker run -it --rm -v $(pwd):/core -w /core ubuntu:22.04 bash -c "
    apt-get update && \
    apt-get install -y build-essential git autoconf automake libtool pkg-config \
        python3-dev bison flex libx11-dev libxml2-dev make && \
    ./autogen.sh --without-java --without-help --without-myspell-dicts && \
    make
"
```

### 2. Run LibreOffice from Docker

```bash
# Run with X11 forwarding (Linux)
docker run -it --rm \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v $(pwd):/core -w /core \
    ubuntu:22.04 \
    /core/instdir/program/soffice
```

## Dockerfile for Build Environment

Create a `Dockerfile` in this directory:

```dockerfile
# Dockerfile - Ubuntu-based LibreOffice build environment
# Note: LibreOffice requires GCC 12+ (Ubuntu 22.04 needs PPA, Ubuntu 24.04 has it by default)
FROM ubuntu:22.04

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install basic build tools
RUN apt-get update && apt-get install -y \
    # Basic tools
    build-essential \
    git \
    autoconf \
    automake \
    libtool \
    pkg-config \
    ccache \
    # Python and Java
    python3-dev \
    python3-pip \
    openjdk-11-jdk \
    ant \
    # Build dependencies
    bison \
    flex \
    gperf \
    nasm \
    libxml2-utils \
    xsltproc \
    # Development libraries
    libx11-dev \
    libxext-dev \
    libxrender-dev \
    libxrandr-dev \
    libxtst-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libgtk-3-dev \
    libcups2-dev \
    libfontconfig1-dev \
    libxinerama-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    # More dependencies
    libcairo2-dev \
    libkrb5-dev \
    libnss3-dev \
    libxml2-dev \
    libxslt1-dev \
    libpython3-dev \
    libboost-dev \
    libhunspell-dev \
    libhyphen-dev \
    libmythes-dev \
    liblpsolve55-dev \
    libcppunit-dev \
    libclucene-dev \
    libexpat1-dev \
    libmysqlclient-dev \
    libpq-dev \
    firebird-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    # Utilities
    wget \
    curl \
    zip \
    unzip \
    sudo \
    locales \
    && rm -rf /var/lib/apt/lists/*

# Generate locale
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US:en
ENV LC_ALL=en_US.UTF-8

# Set up ccache
RUN mkdir -p /ccache && chmod 777 /ccache
ENV CCACHE_DIR=/ccache
ENV CCACHE_MAXSIZE=5G
ENV PATH="/usr/lib/ccache:${PATH}"

# Create build user
RUN useradd -m -s /bin/bash builder && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Set Java environment
ENV JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64

# Switch to builder user
USER builder
WORKDIR /home/builder

# Set up build environment
ENV PARALLELISM=4

# Entry point
CMD ["/bin/bash"]
```

### Alternative: Fedora-based Build

Create `Dockerfile.fedora`:

```dockerfile
# Dockerfile.fedora - Fedora-based build environment
FROM fedora:38

# Install build dependencies
RUN dnf install -y \
    # Development tools
    @development-tools \
    git \
    ccache \
    # Build requirements
    autoconf \
    automake \
    libtool \
    pkg-config \
    bison \
    flex \
    gperf \
    nasm \
    # Libraries
    gtk3-devel \
    cups-devel \
    libX11-devel \
    libXext-devel \
    libXrender-devel \
    libXrandr-devel \
    libXinerama-devel \
    mesa-libGL-devel \
    mesa-libGLU-devel \
    cairo-devel \
    gstreamer1-devel \
    gstreamer1-plugins-base-devel \
    # More dependencies
    python3-devel \
    java-11-openjdk-devel \
    ant \
    boost-devel \
    hunspell-devel \
    hyphen-devel \
    mythes-devel \
    lpsolve-devel \
    cppunit-devel \
    libcurl-devel \
    openssl-devel \
    postgresql-devel \
    mariadb-connector-c-devel \
    firebird-devel \
    && dnf clean all

# Set up environment
ENV JAVA_HOME=/usr/lib/jvm/java-11-openjdk
ENV CCACHE_DIR=/ccache
ENV CCACHE_MAXSIZE=5G

# Create build user
RUN useradd -m builder
USER builder
WORKDIR /home/builder

CMD ["/bin/bash"]
```

### Minimal Build Environment

Create `Dockerfile.minimal` for faster builds:

```dockerfile
# Dockerfile.minimal - Minimal build environment for fast builds
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

# Install only essential build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    autoconf \
    automake \
    libtool \
    pkg-config \
    python3-dev \
    bison \
    flex \
    libx11-dev \
    libxml2-dev \
    libxslt1-dev \
    libgtk-3-dev \
    libcups2-dev \
    libfontconfig1-dev \
    zip \
    ccache \
    && rm -rf /var/lib/apt/lists/*

ENV CCACHE_DIR=/ccache
ENV PATH="/usr/lib/ccache:${PATH}"

USER nobody
WORKDIR /lo

CMD ["/bin/bash"]
```

## Docker Compose Setup

Create `docker-compose.yml` in this directory:

```yaml
version: '3.8'

services:
  # Main build environment
  builder:
    build:
      context: .
      dockerfile: Dockerfile
    image: libreoffice-builder:latest
    container_name: lo-builder
    volumes:
      - .:/core
      - ccache:/ccache
      - maven-cache:/home/builder/.m2
    working_dir: /core
    environment:
      - PARALLELISM=8
      - CCACHE_DIR=/ccache
      - DISPLAY=${DISPLAY}
    network_mode: host
    stdin_open: true
    tty: true
    command: /bin/bash

  # Development environment with IDE support
  dev:
    build:
      context: .
      dockerfile: Dockerfile.dev
    image: libreoffice-dev:latest
    container_name: lo-dev
    volumes:
      - .:/core
      - ccache:/ccache
      - vscode-extensions:/home/builder/.vscode-server
    working_dir: /core
    environment:
      - DISPLAY=${DISPLAY}
    network_mode: host
    stdin_open: true
    tty: true

  # Test runner
  tester:
    build:
      context: .
      dockerfile: Dockerfile
    image: libreoffice-builder:latest
    container_name: lo-tester
    volumes:
      - .:/core
      - test-results:/core/test-results
    working_dir: /core
    command: make check

  # Documentation builder
  docs:
    build:
      context: .
      dockerfile: Dockerfile
    image: libreoffice-builder:latest
    container_name: lo-docs
    volumes:
      - .:/core
      - docs-output:/core/docs-output
    working_dir: /core
    command: make docs

volumes:
  ccache:
  maven-cache:
  vscode-extensions:
  test-results:
  docs-output:
```

### Development Environment

Create `Dockerfile.dev` for development with debugging tools:

```dockerfile
# Dockerfile.dev - Development environment with debugging tools
FROM libreoffice-builder:latest

USER root

# Install debugging and development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    strace \
    ltrace \
    vim \
    emacs \
    tmux \
    htop \
    clang \
    clang-tools \
    clang-format \
    clang-tidy \
    bear \
    universal-ctags \
    cscope \
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Install code analysis tools
RUN pip3 install \
    pylint \
    flake8 \
    black \
    mypy \
    cppcheck

# Set up debugging environment
RUN echo "set auto-load safe-path /" >> /home/builder/.gdbinit && \
    echo "set print pretty on" >> /home/builder/.gdbinit

# Install VS Code server dependencies
RUN apt-get update && apt-get install -y \
    libxkbfile1 \
    libsecret-1-0 \
    libgbm1 \
    && rm -rf /var/lib/apt/lists/*

USER builder

# Set up development environment
RUN echo 'export PS1="\[\033[01;32m\]lo-dev\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ "' >> ~/.bashrc

CMD ["/bin/bash"]
```

## Usage Examples

### 1. Basic Build Workflow

From this directory (core-master):

```bash
# Build the Docker image
docker build -f Dockerfile -t libreoffice-builder .

# Run interactive build
docker run -it --rm \
    -v $(pwd):/core \
    -v lo-ccache:/ccache \
    -w /core \
    libreoffice-builder \
    bash

# Inside container
./autogen.sh --enable-debug --without-java --without-help
make -j$(nproc)
```

### 2. Using Docker Compose

```bash
# Start build environment
docker-compose up -d builder

# Enter build container
docker-compose exec builder bash

# Configure and build
./autogen.sh --enable-debug --enable-dbgutil
make -j$(nproc)

# Run tests
docker-compose run --rm tester

# Stop containers
docker-compose down
```

### 3. Automated Build Script

Create `docker-build.sh` in this directory:

```bash
#!/bin/bash
# docker-build.sh - Automated Docker build script for LibreOffice

set -e

# Configuration
IMAGE_NAME="libreoffice-builder"
CONTAINER_NAME="lo-build-$$"
CCACHE_VOLUME="lo-ccache"

# Build options
BUILD_TYPE="${BUILD_TYPE:-debug}"
PARALLELISM="${PARALLELISM:-$(nproc)}"

# Autogen options based on build type
case "$BUILD_TYPE" in
    debug)
        AUTOGEN_OPTS="--enable-debug --enable-dbgutil --without-java --without-help"
        ;;
    release)
        AUTOGEN_OPTS="--enable-release-build"
        ;;
    minimal)
        AUTOGEN_OPTS="--disable-debug --without-java --without-help --without-myspell-dicts"
        ;;
    *)
        echo "Unknown build type: $BUILD_TYPE"
        exit 1
        ;;
esac

# Build Docker image if needed
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "Building Docker image..."
    docker build -f Dockerfile -t "$IMAGE_NAME" .
fi

# Create ccache volume if needed
if ! docker volume inspect "$CCACHE_VOLUME" >/dev/null 2>&1; then
    echo "Creating ccache volume..."
    docker volume create "$CCACHE_VOLUME"
fi

# Run build
echo "Starting build (type: $BUILD_TYPE)..."
docker run --rm \
    --name "$CONTAINER_NAME" \
    -v "$(pwd):/core" \
    -v "$CCACHE_VOLUME:/ccache" \
    -w /core \
    -e "PARALLELISM=$PARALLELISM" \
    "$IMAGE_NAME" \
    bash -c "
        echo '=== Configuring LibreOffice ==='
        ./autogen.sh $AUTOGEN_OPTS
        
        echo '=== Building LibreOffice ==='
        make -j\$PARALLELISM
        
        echo '=== Build complete ==='
        echo 'Run with: instdir/program/soffice'
    "
```

Make it executable:
```bash
chmod +x docker-build.sh
```

### 4. GUI Support

#### Linux (X11)
```bash
# Allow X11 connections
xhost +local:docker

# Run with GUI
docker run -it --rm \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -v $(pwd):/core \
    -w /core \
    libreoffice-builder \
    bash -c "./autogen.sh --without-java --without-help && make && instdir/program/soffice"
```

#### macOS (XQuartz)
```bash
# Install XQuartz
brew install --cask xquartz

# Start XQuartz and allow connections
open -a XQuartz
# In XQuartz preferences, check "Allow connections from network clients"

# Get IP
IP=$(ifconfig en0 | grep inet | awk '$1=="inet" {print $2}')

# Run with GUI
docker run -it --rm \
    -e DISPLAY=$IP:0 \
    -v $(pwd):/core \
    -w /core \
    libreoffice-builder \
    bash -c "./autogen.sh --without-java --without-help && make && instdir/program/soffice"
```

#### Windows (WSL2 + X Server)
```powershell
# Install VcXsrv or X410
# Configure X server to allow connections

# In WSL2
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0

# Run Docker
docker run -it --rm \
    -e DISPLAY=$DISPLAY \
    -v $(pwd):/core \
    -w /core \
    libreoffice-builder \
    bash -c "./autogen.sh --without-java --without-help && make && instdir/program/soffice"
```

### 5. Development Workflow

Create `.devcontainer/devcontainer.json` for VS Code:

```json
{
    "name": "LibreOffice Development",
    "dockerComposeFile": "../docker-compose.yml",
    "service": "dev",
    "workspaceFolder": "/core",
    "shutdownAction": "none",
    
    "customizations": {
        "vscode": {
            "extensions": [
                "ms-vscode.cpptools",
                "ms-python.python",
                "ms-vscode.cmake-tools",
                "vadimcn.vscode-lldb"
            ],
            "settings": {
                "terminal.integrated.defaultProfile.linux": "bash",
                "C_Cpp.default.compilerPath": "/usr/bin/g++",
                "C_Cpp.default.intelliSenseMode": "linux-gcc-x64"
            }
        }
    },
    
    "forwardPorts": [],
    "postCreateCommand": "./autogen.sh --enable-debug --enable-dbgutil",
    "remoteUser": "builder"
}
```

## Advanced Docker Configurations

### Multi-Stage Build

Create `Dockerfile.multistage`:

```dockerfile
# Stage 1: Build dependencies
FROM ubuntu:22.04 AS deps
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    wget \
    && wget -O packages.txt https://raw.githubusercontent.com/LibreOffice/core/master/distro-configs/LibreOfficeLinux.conf \
    && apt-get install -y $(cat packages.txt | grep -v '^#' | xargs) \
    && rm -rf /var/lib/apt/lists/*

# Stage 2: Build environment
FROM deps AS builder
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    ccache \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /core

# Stage 3: Runtime
FROM ubuntu:22.04 AS runtime
COPY --from=builder /core/instdir /opt/libreoffice
ENV PATH="/opt/libreoffice/program:${PATH}"
CMD ["soffice"]
```

### CI/CD Pipeline

Create `.gitlab-ci.yml`:

```yaml
image: libreoffice-builder:latest

variables:
  CCACHE_DIR: "/ccache"
  
cache:
  key: "$CI_COMMIT_REF_SLUG"
  paths:
    - ccache/

stages:
  - build
  - test
  - package

build:
  stage: build
  script:
    - ./autogen.sh --enable-debug --without-java
    - make -j$(nproc)
  artifacts:
    paths:
      - instdir/
    expire_in: 1 day

test:
  stage: test
  dependencies:
    - build
  script:
    - make check
  artifacts:
    reports:
      junit: workdir/junit/*.xml

package:
  stage: package
  dependencies:
    - build
  script:
    - make distro-pack-install
  artifacts:
    paths:
      - workdir/installation/
```

## Quick Build Commands

### One-Liner Builds

```bash
# Minimal debug build (fastest)
docker run -it --rm -v $(pwd):/core -w /core ubuntu:22.04 bash -c "
    apt-get update && apt-get install -y build-essential git autoconf automake \
    libtool pkg-config python3-dev bison flex libx11-dev libxml2-dev make && \
    ./autogen.sh --without-java --without-help --without-myspell-dicts && make
"

# Full featured build
docker build -t lo-builder . && docker run -it --rm -v $(pwd):/core -w /core lo-builder

# Build with output
docker run -it --rm -v $(pwd):/core -v $(pwd)/instdir:/output -w /core ubuntu:22.04 \
    bash -c "apt-get update && apt-get build-dep -y libreoffice && \
    ./autogen.sh && make && cp -r instdir/* /output/"
```

## Best Practices

### 1. Cache Management

```bash
# Create persistent ccache volume
docker volume create lo-ccache

# Use in all builds
-v lo-ccache:/ccache

# Check cache stats
docker run --rm -v lo-ccache:/ccache libreoffice-builder ccache -s

# Clear cache
docker run --rm -v lo-ccache:/ccache libreoffice-builder ccache -C
```

### 2. Resource Limits

```bash
# Limit CPU and memory
docker run -it --rm \
    --cpus="4" \
    --memory="8g" \
    --memory-swap="8g" \
    -v $(pwd):/core -w /core \
    libreoffice-builder
```

### 3. Build Optimization

```dockerfile
# Add to Dockerfile for faster builds
# Use BuildKit
# syntax=docker/dockerfile:1

# Cache mount for apt
RUN --mount=type=cache,target=/var/cache/apt \
    --mount=type=cache,target=/var/lib/apt \
    apt-get update && apt-get install -y ...

# Cache mount for ccache
RUN --mount=type=cache,target=/ccache \
    make -j$(nproc)
```

### 4. Security

```dockerfile
# Run as non-root user
USER builder

# Use read-only root filesystem
# In docker-compose.yml:
read_only: true
tmpfs:
  - /tmp
  - /run
```

## Troubleshooting

### Common Issues

1. **Permission Errors**
```bash
# Fix ownership
docker run --rm -v $(pwd):/core ubuntu chown -R $(id -u):$(id -g) /core
```

2. **Out of Space**
```bash
# Clean up Docker
docker system prune -a
docker volume prune
```

3. **Slow Builds**
```bash
# Increase Docker resources
# Docker Desktop: Settings > Resources
# Increase CPUs and Memory
```

4. **X11 Connection Refused**
```bash
# Linux
xhost +local:docker

# macOS with XQuartz
defaults write org.xquartz.X11 enable_iglx -bool true
```

## Summary

### Quickest Start

From this LibreOffice core directory:

```bash
# Fast minimal build
docker run -it --rm -v $(pwd):/core -w /core ubuntu:22.04 bash -c "
    apt-get update && apt-get install -y build-essential autoconf automake \
    libtool pkg-config python3-dev bison flex libx11-dev libxml2-dev make && \
    ./autogen.sh --without-java --without-help --without-myspell-dicts && \
    make && echo 'Done! Run: instdir/program/soffice'
"
```

### For Development

1. Create the `Dockerfile` in this directory
2. Build image: `docker build -t lo-dev .`
3. Run: `docker run -it -v $(pwd):/core -w /core lo-dev`
4. Inside container: `./autogen.sh && make`

The Docker approach eliminates all dependency issues and provides a consistent build environment across all platforms.