#!/bin/bash
# build-libreoffice.sh - Build LibreOffice from source

set -e

echo "============================================"
echo "Building LibreOffice from Source"
echo "============================================"

# Ensure build directory exists
mkdir -p /build
cd /build

# Check if source files exist and copy if needed
if [ ! -f ./configure.ac ]; then
    echo "Copying LibreOffice source files from /core/..."
    cp -r /core/* /build/ 2>/dev/null || true
    chown -R builder:builder /build
fi

echo "Build directory: $(pwd)"
echo "Parallelism: ${PARALLELISM:-4}"

# Display disk space
echo "Available disk space:"
df -h /build

# Configure build if not already configured
if [ ! -f ./config_host.mk ]; then
    echo ""
    echo "============================================"
    echo "Configuring LibreOffice Build..."
    echo "============================================"
    
    # Run autogen.sh with development-friendly options
    ./autogen.sh \
        --enable-debug \
        --enable-dbgutil \
        --enable-symbols \
        --disable-online-update \
        --disable-community-flavor \
        --without-java \
        --without-help \
        --without-myspell-dicts \
        --with-parallelism=${PARALLELISM:-4} \
        --with-vendor="LibreOffice VNC Development Build"
    
    echo "Configuration complete!"
else
    echo "Build already configured, skipping autogen.sh"
fi

echo ""
echo "============================================"
echo "Building LibreOffice..."
echo "============================================"
echo "This will take 30-60 minutes depending on your system"
echo "You can monitor progress in this terminal"
echo ""

# Start the build
time make -j${PARALLELISM:-4}

echo ""
echo "============================================"
echo "Build Complete!"
echo "============================================"
echo "LibreOffice binaries are available at:"
echo "  /build/instdir/program/soffice"
echo ""
echo "To run LibreOffice applications:"
echo "  Writer: /build/instdir/program/soffice --writer"
echo "  Calc:   /build/instdir/program/soffice --calc"
echo "  Draw:   /build/instdir/program/soffice --draw"
echo "  Impress:/build/instdir/program/soffice --impress"
echo ""
echo "Or use the desktop shortcuts in the VNC session!"
echo "============================================"