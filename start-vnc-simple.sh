#!/bin/bash
# start-vnc-simple.sh - Simple VNC startup script

set -e

echo "============================================"
echo "LibreOffice VNC Environment Starting"
echo "============================================"

# Set default values
VNC_RESOLUTION=${VNC_RESOLUTION:-1920x1080}
DISPLAY=${DISPLAY:-:1}

echo "VNC Resolution: $VNC_RESOLUTION"
echo "Display: $DISPLAY"

# Kill any existing VNC servers
vncserver -kill $DISPLAY 2>/dev/null || true

# Start VNC server
echo "Starting VNC server on display $DISPLAY..."
vncserver $DISPLAY -geometry $VNC_RESOLUTION -depth 24 -dpi 96

# Wait for VNC server to start
sleep 3

# Start noVNC in the background
echo "Starting noVNC web interface on port 6080..."
cd /usr/share/novnc
./utils/launch.sh --vnc localhost:5901 --listen 6080 &

# Copy source files to build directory if needed
if [ ! -f /build/.initialized ]; then
    echo "First time setup - copying source files..."
    mkdir -p /build
    cp -r /core/* /build/ 2>/dev/null || true
    touch /build/.initialized
    chown -R builder:builder /build
fi

echo ""
echo "============================================"
echo "LibreOffice VNC Environment Ready!"
echo "============================================"
echo "VNC Server: localhost:5901"
echo "Web Interface: http://localhost:6080/vnc.html"
echo "Password: $VNC_PASSWORD"
echo ""
echo "To connect:"
echo "1. Open browser: http://localhost:6080/vnc.html"
echo "2. Or use VNC client: localhost:5901"
echo "============================================"

# Keep container running
tail -f /dev/null