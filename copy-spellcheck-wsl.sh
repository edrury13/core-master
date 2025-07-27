#!/bin/bash
# Script to copy spell check improvements to WSL LibreOffice build directory

# Source files (Windows paths)
declare -a files=(
    "editeng/source/editeng/editview.cxx"
    "sw/source/core/edit/edlingu.cxx"
    "cui/source/dialogs/SpellDialog.cxx"
    "cui/source/inc/SpellDialog.hxx"
    "cui/uiconfig/ui/spellingdialog.ui"
    "cui/uiconfig/ui/learnfromdocdialog.ui"
    "cui/inc/strings.hrc"
)

SOURCE_DIR="/mnt/c/Users/drury/Documents/GitHub/core-master"
DEST_DIR="/home/drury/libreoffice/core-master"

echo "Copying spell check improvements..."

success_count=0
error_count=0

for file in "${files[@]}"; do
    src="$SOURCE_DIR/$file"
    dest="$DEST_DIR/$file"
    dest_dir=$(dirname "$dest")
    
    # Create destination directory if it doesn't exist
    mkdir -p "$dest_dir"
    
    if cp "$src" "$dest"; then
        echo "✓ Copied: $file"
        ((success_count++))
    else
        echo "✗ Failed to copy: $file"
        ((error_count++))
    fi
done

echo ""
echo "Summary:"
echo "Successfully copied: $success_count files"
if [ $error_count -gt 0 ]; then
    echo "Failed to copy: $error_count files"
fi

echo ""
echo "Next steps:"
echo "1. cd ~/libreoffice/core-master"
echo "2. make cui.clean && make cui"
echo "3. make editeng.clean && make editeng"
echo "4. make sw.clean && make sw"
echo "5. make check"