#!/bin/bash
# Check if CUI module builds with our changes

cd ~/libreoffice/core-master

echo "Building CUI module..."
make cui.build 2>&1 | tee cui-build.log

# Check for errors
if grep -q "Error" cui-build.log; then
    echo "Build errors found in CUI module:"
    grep -A5 -B5 "error:" cui-build.log
else
    echo "CUI module built successfully!"
fi