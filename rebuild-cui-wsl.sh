#!/bin/bash
# Rebuild CUI module in WSL

echo "Rebuilding CUI module in LibreOffice..."
echo "======================================="

cd ~/libreoffice/core-master

# Clean and rebuild CUI module
echo "Cleaning CUI module..."
make cui.clean

echo ""
echo "Building CUI module..."
make cui 2>&1 | tee cui-build.log

# Check if build succeeded
if [ ${PIPESTATUS[0]} -eq 0 ]; then
    echo ""
    echo "Build succeeded!"
    echo "You can now run LibreOffice with: ./instdir/program/soffice --writer"
else
    echo ""
    echo "Build failed! Check cui-build.log for errors."
    echo "Showing last 50 lines of error log:"
    tail -50 cui-build.log
fi