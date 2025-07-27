#!/bin/bash

echo "=== FIXING GOOGLE DRIVE FUNCTIONALITY ==="
echo ""

# Define paths
WINDOWS_BASE="/mnt/c/Users/drury/Documents/GitHub/core-master"
WSL_BASE="$HOME/libreoffice/core-master"

# Function to copy and verify
copy_and_verify() {
    local src="$1"
    local dst="$2"
    local file=$(basename "$src")
    
    echo -n "Copying $file... "
    cp "$WINDOWS_BASE/$src" "$WSL_BASE/$dst" 2>/dev/null
    
    if [ -f "$WSL_BASE/$dst/$file" ]; then
        echo "✓"
        return 0
    else
        echo "✗ FAILED"
        return 1
    fi
}

# Copy all Google Drive files
echo "1. Copying Google Drive files..."
copy_and_verify "cui/source/dialogs/GoogleDriveFilePicker.cxx" "cui/source/dialogs"
copy_and_verify "cui/source/dialogs/GoogleDriveFilePicker.hxx" "cui/source/dialogs"
copy_and_verify "cui/source/dialogs/GoogleDriveFilePickerService.cxx" "cui/source/dialogs"
copy_and_verify "cui/source/dialogs/GoogleDriveFilePickerService.hxx" "cui/source/dialogs"

# Copy UCP files
echo ""
echo "2. Copying UCP files..."
copy_and_verify "ucb/source/ucp/gdocs/gdocs_auth.cxx" "ucb/source/ucp/gdocs"
copy_and_verify "ucb/source/ucp/gdocs/gdocs_datasupplier.cxx" "ucb/source/ucp/gdocs"
copy_and_verify "ucb/source/ucp/gdocs/gdocs_content.cxx" "ucb/source/ucp/gdocs"
copy_and_verify "ucb/source/ucp/gdocs/gdocs_provider.cxx" "ucb/source/ucp/gdocs"

# Copy SFX2 files
echo ""
echo "3. Copying SFX2 files..."
copy_and_verify "sfx2/source/appl/appopen.cxx" "sfx2/source/appl"

# Verify debug code is present
echo ""
echo "4. Verifying debug code..."
cd "$WSL_BASE"

echo -n "GoogleDriveFilePicker constructor debug: "
if grep -q "GoogleDriveFilePicker CONSTRUCTOR" cui/source/dialogs/GoogleDriveFilePicker.cxx; then
    echo "✓ FOUND"
else
    echo "✗ MISSING"
fi

echo -n "GoogleDriveFilePickerService debug: "
if grep -q "GoogleDriveFilePickerService::execute() CALLED" cui/source/dialogs/GoogleDriveFilePickerService.cxx; then
    echo "✓ FOUND"
else
    echo "✗ MISSING"
fi

echo -n "SFX2 appopen debug: "
if grep -q "OpenFromGoogleDriveExec_Impl called" sfx2/source/appl/appopen.cxx; then
    echo "✓ FOUND"
else
    echo "✗ MISSING"
fi

# Clean old objects
echo ""
echo "5. Cleaning old object files..."
rm -f workdir/CxxObject/cui/source/dialogs/GoogleDriveFilePicker.o
rm -f workdir/CxxObject/cui/source/dialogs/GoogleDriveFilePickerService.o
rm -f workdir/CxxObject/sfx2/source/appl/appopen.o
rm -f workdir/CxxObject/ucb/source/ucp/gdocs/*.o

# Remove old debug log
rm -f /tmp/gdrive_debug.log

echo ""
echo "6. Ready to build. Run these commands:"
echo "   cd $WSL_BASE"
echo "   make sfx2 cui ucb"
echo ""
echo "7. After building, run:"
echo "   ./instdir/program/soffice"
echo ""
echo "8. Check for debug output:"
echo "   - Watch the terminal for fprintf output"
echo "   - After clicking Google Drive: cat /tmp/gdrive_debug.log"