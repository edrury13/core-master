#!/bin/bash
# Apply all fixes to WSL

echo "Applying all UCB fixes..."

WIN_SOURCE="/mnt/c/Users/drury/Documents/GitHub/core-master"
WSL_DEST="$HOME/libreoffice/core-master"

# Copy fixed files
echo "Copying fixed gdocs_auth.cxx..."
cp -v "$WIN_SOURCE/ucb/source/ucp/gdocs/gdocs_auth_fixed.cxx" "$WSL_DEST/ucb/source/ucp/gdocs/gdocs_auth.cxx"

# Run the content provider fixes
echo "Running content provider fixes..."
cd "$WIN_SOURCE"
chmod +x fix-content-provider.sh
./fix-content-provider.sh

# Update the Library makefile to add missing libraries
echo "Updating Library_ucpgdocs1.mk..."
sed -i '/comphelper \\/a\    o3tl \\' "$WSL_DEST/ucb/Library_ucpgdocs1.mk"

echo "All fixes applied!"
echo ""
echo "Now try building again with:"
echo "cd ~/libreoffice/core-master"
echo "make ucb.clean && make ucb"