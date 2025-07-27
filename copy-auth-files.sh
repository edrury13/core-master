#!/bin/bash
# Script to copy Google Drive authentication changes within WSL

echo "Copying Google Drive Unified Authentication changes..."

# Windows source path
WIN_SOURCE="/mnt/c/Users/drury/Documents/GitHub/core-master"
# WSL destination path
WSL_DEST="$HOME/libreoffice/core-master"

# List of files to copy
FILES=(
    "include/ucb/gdocsauth.hxx"
    "ucb/source/ucp/gdocs/gdocs_authservice.cxx"
    "cui/source/dialogs/GoogleDriveFilePicker.cxx"
    "ucb/source/ucp/gdocs/gdocs_auth.cxx"
    "ucb/Library_ucpgdocs1.mk"
)

# Create directories if they don't exist
mkdir -p "$WSL_DEST/include/ucb"
mkdir -p "$WSL_DEST/ucb/source/ucp/gdocs"
mkdir -p "$WSL_DEST/cui/source/dialogs"

# Copy each file
for file in "${FILES[@]}"; do
    if [ -f "$WIN_SOURCE/$file" ]; then
        cp -v "$WIN_SOURCE/$file" "$WSL_DEST/$file"
    else
        echo "Warning: Source file not found: $file"
    fi
done

echo ""
echo "Done! Files copied to WSL."
echo ""
echo "Next steps:"
echo "1. cd ~/libreoffice/core-master"
echo "2. make ucb.clean && make ucb"
echo "3. make cui.clean && make cui"
echo "4. make"