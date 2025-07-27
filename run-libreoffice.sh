#!/bin/bash
# run-libreoffice.sh - Run LibreOffice applications from VNC environment

set -e

# Application to run (writer, calc, draw, impress, or base)
APP=${1:-writer}

echo "============================================"
echo "Running LibreOffice $APP"
echo "============================================"

# Check if LibreOffice is built
if [ ! -f /build/instdir/program/soffice ]; then
    echo "LibreOffice not found at /build/instdir/program/soffice"
    echo "Building LibreOffice first..."
    echo ""
    
    # Run the build script
    /home/builder/build-libreoffice.sh
fi

echo "Starting LibreOffice $APP..."
echo "Binary: /build/instdir/program/soffice"

# Set up environment for LibreOffice
export SAL_USE_VCLPLUGIN=gtk3
export DISPLAY=:1

# Create a user profile directory if it doesn't exist
mkdir -p /home/builder/.config/libreoffice

# Ensure we're in the build directory
mkdir -p /build
cd /build

# Run LibreOffice with the specified application
case "$APP" in
    writer)
        echo "Starting LibreOffice Writer..."
        exec ./instdir/program/soffice --writer
        ;;
    calc)
        echo "Starting LibreOffice Calc..."
        exec ./instdir/program/soffice --calc
        ;;
    draw)
        echo "Starting LibreOffice Draw..."
        exec ./instdir/program/soffice --draw
        ;;
    impress)
        echo "Starting LibreOffice Impress..."
        exec ./instdir/program/soffice --impress
        ;;
    base)
        echo "Starting LibreOffice Base..."
        exec ./instdir/program/soffice --base
        ;;
    math)
        echo "Starting LibreOffice Math..."
        exec ./instdir/program/soffice --math
        ;;
    *)
        echo "Starting LibreOffice (default)..."
        exec ./instdir/program/soffice
        ;;
esac