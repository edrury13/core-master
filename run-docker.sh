#!/bin/bash
# run-docker.sh - Run LibreOffice VNC environment

set -e

IMAGE_NAME="libreoffice-builder-vnc"
CONTAINER_NAME="libreoffice-vnc"

echo "============================================"
echo "Starting LibreOffice VNC Environment"
echo "============================================"

# Check if image exists
if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "Docker image not found. Building it first..."
    ./build-docker.sh
fi

# Stop and remove existing container if it exists
if docker ps -a --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Stopping existing container: $CONTAINER_NAME"
    docker stop "$CONTAINER_NAME" >/dev/null 2>&1 || true
    docker rm "$CONTAINER_NAME" >/dev/null 2>&1 || true
fi

# Run the container
echo "Starting container: $CONTAINER_NAME"
docker run -d \
    --name "$CONTAINER_NAME" \
    -p 5901:5901 \
    -p 6080:6080 \
    -v "$(pwd):/core:ro" \
    -v "libreoffice-build:/build" \
    -v "libreoffice-ccache:/ccache" \
    -v "libreoffice-home:/home/builder" \
    -e VNC_RESOLUTION=1920x1080 \
    -e VNC_PASSWORD=libreoffice \
    -e PARALLELISM=4 \
    --restart unless-stopped \
    "$IMAGE_NAME"

# Wait for services to start
echo "Waiting for services to start..."
sleep 10

# Display connection information
echo ""
echo "============================================"
echo "LibreOffice VNC Environment Started!"
echo "============================================"
echo "Container: $CONTAINER_NAME"
echo ""
echo "Connection Options:"
echo "1. VNC Client: localhost:5901"
echo "2. Web Browser: http://localhost:6080/vnc.html"
echo "   Password: libreoffice"
echo ""
echo "Desktop Environment: XFCE4"
echo "LibreOffice Source: /core (read-only)"
echo "Build Directory: /build"
echo ""
echo "Available desktop shortcuts:"
echo "- Build LibreOffice"
echo "- LibreOffice Writer"
echo "- LibreOffice Calc"
echo ""
echo "To view logs:"
echo "  docker logs -f $CONTAINER_NAME"
echo ""
echo "To stop:"
echo "  docker stop $CONTAINER_NAME"
echo "============================================"