#!/bin/bash
# Script to copy JSON parser fixes to WSL

echo "Copying JSON parser fixes..."

# Windows source path
WIN_SOURCE="/mnt/c/Users/drury/Documents/GitHub/core-master"
# WSL destination path
WSL_DEST="$HOME/libreoffice/core-master"

# Copy the updated files
cp -v "$WIN_SOURCE/ucb/source/ucp/gdocs/gdocs_authservice.cxx" "$WSL_DEST/ucb/source/ucp/gdocs/"
cp -v "$WIN_SOURCE/ucb/source/ucp/gdocs/gdocs_json.hxx" "$WSL_DEST/ucb/source/ucp/gdocs/"
cp -v "$WIN_SOURCE/ucb/Library_ucpgdocs1.mk" "$WSL_DEST/ucb/"

echo "Done!"