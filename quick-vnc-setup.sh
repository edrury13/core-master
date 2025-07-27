#!/bin/bash
# quick-vnc-setup.sh - Minimal VNC setup for existing container

set -e

# Install only essential VNC packages
apt-get update
apt-get install -y --no-install-recommends \
    tigervnc-standalone-server \
    x11-xserver-utils \
    xfonts-base \
    xterm \
    novnc \
    websockify

# Create VNC directory and password
mkdir -p ~/.vnc
echo "libreoffice" | vncpasswd -f > ~/.vnc/passwd
chmod 600 ~/.vnc/passwd

# Create simple xstartup script (no desktop environment, just basic X11)
cat > ~/.vnc/xstartup << 'EOF'
#!/bin/bash
export DISPLAY=:1
xrdb ~/.Xresources 2>/dev/null || true
xterm -geometry 80x24+100+100 &
# Keep session alive
exec tail -f /dev/null
EOF

chmod +x ~/.vnc/xstartup

# Start VNC server
vncserver :1 -geometry 1920x1080 -depth 24 -dpi 96

echo "VNC server started on display :1"
echo "Starting noVNC web interface..."

# Start noVNC
cd /usr/share/novnc
./utils/launch.sh --vnc localhost:5901 --listen 6080 &

echo "Setup complete!"
echo "VNC: localhost:5901"
echo "Web: http://localhost:6080/vnc.html"
echo "Password: libreoffice"