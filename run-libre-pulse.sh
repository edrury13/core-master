#!/bin/bash

echo "Setting up PulseAudio environment..."

# Set PulseAudio runtime path for WSLg
export PULSE_RUNTIME_PATH=/mnt/wslg/runtime-dir/pulse

# Check if PulseAudio is accessible
if [ -S "$PULSE_RUNTIME_PATH/native" ]; then
    echo "✓ PulseAudio socket found at $PULSE_RUNTIME_PATH/native"
else
    echo "✗ PulseAudio socket not found"
    echo "Checking alternative locations..."
    
    # Try other common locations
    for path in /run/user/$(id -u)/pulse /tmp/pulse-* /var/run/pulse; do
        if [ -d "$path" ]; then
            echo "Found pulse directory at: $path"
            export PULSE_RUNTIME_PATH="$path"
            break
        fi
    done
fi

# Start PulseAudio if not running
if ! pactl info &> /dev/null; then
    echo "Starting PulseAudio..."
    pulseaudio --start --log-target=syslog --daemonize=yes
    sleep 2
fi

# Show PulseAudio info
echo -e "\nPulseAudio status:"
pactl info 2>/dev/null || echo "PulseAudio not available"

# Check for audio sources
echo -e "\nAudio sources:"
pactl list sources short 2>/dev/null || echo "No sources found"

# Run LibreOffice
echo -e "\nStarting LibreOffice Writer..."
cd /home/drury/libreoffice/core-master
./instdir/program/swriter