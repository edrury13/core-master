#!/bin/bash

echo "Setting up audio support for WSL..."

# Check WSL version
if ! command -v wslinfo &> /dev/null; then
    echo "This appears to be WSL1. Audio support requires WSL2 with WSLg."
    echo "Please upgrade to WSL2 first."
    exit 1
fi

# Check if running in WSLg (GUI support)
if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
    echo "WSLg (GUI support) not detected. Audio requires WSLg."
    echo "Make sure you're running Windows 11 or Windows 10 with WSLg installed."
    exit 1
fi

# Update package list
echo "Updating package list..."
sudo apt-get update

# Install PulseAudio and ALSA plugins
echo "Installing PulseAudio and ALSA plugins..."
sudo apt-get install -y pulseaudio pulseaudio-utils libasound2-plugins

# Check if PulseAudio is running
echo "Checking PulseAudio status..."
if pactl info &> /dev/null; then
    echo "✓ PulseAudio is running"
    pactl info | grep "Server Name"
else
    echo "✗ PulseAudio is not running"
    echo "Trying to start PulseAudio..."
    pulseaudio --start
    sleep 2
    if pactl info &> /dev/null; then
        echo "✓ PulseAudio started successfully"
    else
        echo "✗ Failed to start PulseAudio"
    fi
fi

# Configure ALSA to use PulseAudio
echo "Configuring ALSA to use PulseAudio..."
cat > ~/.asoundrc << 'EOF'
pcm.default {
    type pulse
}
ctl.default {
    type pulse
}
EOF
echo "✓ ALSA configured to use PulseAudio"

# Test audio devices
echo -e "\nAvailable audio sources:"
pactl list sources short

echo -e "\nDefault audio source:"
pactl info | grep "Default Source"

# Create a test recording
echo -e "\nTesting audio recording (3 seconds)..."
if command -v arecord &> /dev/null; then
    arecord -d 3 -f cd test-audio.wav 2>/dev/null
    if [ -f test-audio.wav ]; then
        echo "✓ Audio recording test successful"
        rm test-audio.wav
    else
        echo "✗ Audio recording test failed"
    fi
else
    echo "arecord not found. Install alsa-utils for testing."
fi

echo -e "\nSetup complete!"
echo "To use PulseAudio in LibreOffice, rebuild with PulseAudio support:"
echo "  1. Edit sw/Library_sw.mk to link against PulseAudio"
echo "  2. Define USE_PULSEAUDIO when building"
echo "  3. Or continue using ALSA (it will use PulseAudio through the plugin)"

# Alternative: Windows audio capture
echo -e "\nAlternative: Use Windows-side audio capture"
echo "You can also run LibreOffice on Windows and use native Windows audio."
echo "The Whisper integration will work with Windows audio APIs (WinMM)."