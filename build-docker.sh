#!/bin/bash
# build-docker.sh - Quick build script for LibreOffice VNC environment

set -e

IMAGE_NAME="libreoffice-builder-vnc"
CONTAINER_NAME="libreoffice-vnc"

echo "============================================"
echo "LibreOffice VNC Environment Builder"
echo "============================================"

# Check if Docker is running
if ! docker info >/dev/null 2>&1; then
    echo "Error: Docker is not running or not accessible"
    exit 1
fi

# Build Docker image
echo "Building Docker image: $IMAGE_NAME"
echo "This may take 10-15 minutes for the first build..."
echo ""

time docker build -t "$IMAGE_NAME" .

echo ""
echo "============================================"
echo "Docker Image Built Successfully!"
echo "============================================"
echo "Image: $IMAGE_NAME"
echo ""
echo "To start the VNC environment:"
echo "  ./run-docker.sh"
echo ""
echo "Or use Docker Compose:"
echo "  docker-compose up -d libreoffice-vnc"
echo "============================================"