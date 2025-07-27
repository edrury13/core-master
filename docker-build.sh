#!/bin/bash
# Docker build script for LibreOffice

set -e

echo "=== Setting up build environment ==="

# Create build directory with proper permissions
mkdir -p /build-cache/workdir
cd /core

# Copy source to build directory to avoid permission issues
echo "=== Copying source files to build directory ==="
cp -r . /build-cache/workdir/
cd /build-cache/workdir

# Configure build
echo "=== Configuring LibreOffice build ==="
./autogen.sh --without-java --without-help --without-myspell-dicts --with-parallelism=4

# Build
echo "=== Building LibreOffice ==="
make -j4

echo "=== Build complete ==="
echo "Build artifacts are in /build-cache/workdir/instdir/"
echo "To run LibreOffice: /build-cache/workdir/instdir/program/soffice"