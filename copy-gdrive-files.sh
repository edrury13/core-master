#!/bin/bash
# Simple script to copy Google Drive files to WSL

echo "Copying GoogleDriveFilePicker.cxx..."
cp /mnt/c/Users/drury/Documents/GitHub/core-master/cui/source/dialogs/GoogleDriveFilePicker.cxx ~/libreoffice/core-master/cui/source/dialogs/

echo "Checking if file has debug changes..."
if grep -q "GoogleDriveFilePicker CONSTRUCTOR" ~/libreoffice/core-master/cui/source/dialogs/GoogleDriveFilePicker.cxx; then
    echo "✓ File has debug changes!"
    grep -n "GoogleDriveFilePicker CONSTRUCTOR" ~/libreoffice/core-master/cui/source/dialogs/GoogleDriveFilePicker.cxx
else
    echo "✗ ERROR: File does NOT have debug changes!"
    echo "Showing what's at line 63:"
    sed -n '60,70p' ~/libreoffice/core-master/cui/source/dialogs/GoogleDriveFilePicker.cxx
fi

echo ""
echo "Force rebuilding GoogleDriveFilePicker..."
cd ~/libreoffice/core-master
rm -f workdir/CxxObject/cui/source/dialogs/GoogleDriveFilePicker.o
echo ""
echo "Now run: make cui"