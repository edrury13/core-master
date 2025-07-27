#!/bin/bash

echo "Testing ALSA with PulseAudio..."

# List ALSA PCM devices
echo -e "\nALSA PCM devices:"
aplay -l 2>/dev/null || echo "aplay not found"

# List ALSA capture devices
echo -e "\nALSA capture devices:"
arecord -l 2>/dev/null || echo "arecord not found"

# Test recording with ALSA (through PulseAudio)
echo -e "\nTesting ALSA recording (3 seconds)..."
arecord -D default -d 3 -f cd -t wav test.wav 2>&1 | head -20

# Check if file was created
if [ -f test.wav ]; then
    echo "✓ Recording successful"
    ls -la test.wav
    rm test.wav
else
    echo "✗ Recording failed"
fi

# Show ALSA configuration
echo -e "\nALSA configuration (~/.asoundrc):"
cat ~/.asoundrc 2>/dev/null || echo "No .asoundrc found"

# Check PulseAudio
echo -e "\nPulseAudio status:"
systemctl --user status pulseaudio 2>/dev/null || echo "systemctl not available"

echo -e "\nPulseAudio info:"
pactl info 2>/dev/null | grep -E "(Server|Default)" || echo "pactl not available"