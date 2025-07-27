#!/bin/bash
# start-vnc.sh - Main startup script for LibreOffice VNC environment

set -e

echo "============================================"
echo "LibreOffice VNC Development Environment"
echo "============================================"

# Set default values
VNC_RESOLUTION=${VNC_RESOLUTION:-1920x1080}
VNC_PASSWORD=${VNC_PASSWORD:-libreoffice}

echo "VNC Resolution: $VNC_RESOLUTION"
echo "Display: $DISPLAY"

# Ensure VNC directory exists and has correct permissions
mkdir -p /home/builder/.vnc
chown -R builder:builder /home/builder/.vnc

# Kill any existing VNC servers
vncserver -kill :1 2>/dev/null || true

# Start VNC server
echo "Starting VNC server on display :1..."
vncserver :1 -geometry $VNC_RESOLUTION -depth 24 -dpi 96

# Wait for VNC server to start
sleep 3

# Start noVNC web interface
echo "Starting noVNC web interface on port 6080..."
cd /usr/share/novnc
./utils/launch.sh --vnc localhost:5901 --listen 6080 &

# Display connection information
echo "============================================"
echo "LibreOffice VNC Environment Ready!"
echo "============================================"
echo "VNC Server: localhost:5901"
echo "Web Interface: http://localhost:6080/vnc.html"
echo "Password: $VNC_PASSWORD"
echo ""
echo "Desktop Environment: XFCE4"
echo "LibreOffice Source: /core"
echo "Build Directory: /build"
echo ""
echo "Desktop shortcuts available for:"
echo "- Build LibreOffice (runs autogen.sh and make)"
echo "- LibreOffice Writer"
echo "- LibreOffice Calc"
echo "============================================"

# Copy source files to build directory if not already done
if [ ! -f /build/.initialized ]; then
    echo "First time setup - copying source files to /build/..."
    cp -r /core/* /build/ 2>/dev/null || true
    touch /build/.initialized
    chown -R builder:builder /build
fi

# Keep container running
tail -f /home/builder/.vnc/*.log