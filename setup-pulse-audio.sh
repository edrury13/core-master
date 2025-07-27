#!/bin/bash

# Configure ALSA to use PulseAudio
echo "Configuring ALSA to use PulseAudio..."

# Create .asoundrc to route ALSA through PulseAudio
cat > ~/.asoundrc << 'EOF'
pcm.!default {
    type pulse
}
ctl.!default {
    type pulse
}
EOF

echo "✓ ALSA configured to use PulseAudio"

# Check if PulseAudio is running
if pactl info &> /dev/null; then
    echo "✓ PulseAudio is running"
    echo "Server: $(pactl info | grep 'Server Name' | cut -d: -f2)"
else
    echo "Starting PulseAudio..."
    pulseaudio --start --log-target=syslog
    sleep 2
    if pactl info &> /dev/null; then
        echo "✓ PulseAudio started"
    else
        echo "✗ Failed to start PulseAudio"
    fi
fi

# List audio sources
echo -e "\nAvailable audio sources:"
pactl list sources short

# Set environment variable for LibreOffice
echo -e "\nSetting up environment..."
echo "export PULSE_RUNTIME_PATH=/mnt/wslg/runtime-dir/pulse" >> ~/.bashrc

echo -e "\nSetup complete! ALSA will now use PulseAudio."