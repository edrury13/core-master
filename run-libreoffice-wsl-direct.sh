#!/bin/bash
# Run LibreOffice directly in WSL without Docker

# Check if we're in WSL
if [ ! -f /proc/sys/fs/binfmt_misc/WSLInterop ]; then
    echo "This script must be run inside WSL"
    exit 1
fi

# Set display for X11 forwarding
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0

# Navigate to the build directory
cd /mnt/c/Users/drury/Documents/GitHub/core-master

# Check if LibreOffice is built
if [ -d "instdir/program" ]; then
    echo "Running LibreOffice from instdir..."
    ./instdir/program/soffice
else
    echo "LibreOffice build not found in instdir/program"
    echo "Please build LibreOffice first"
    exit 1
fi