#!/bin/bash
# Run LibreOffice Writer from WSL build for testing

cd /home/drury/libreoffice/core-master

# Run Writer with a test document
echo "Starting LibreOffice Writer for spell check testing..."
./instdir/program/soffice --writer --norestore