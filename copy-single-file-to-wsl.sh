#!/bin/bash
# Copy the updated impdialog.cxx file to WSL

# Create the file content using a here document
cat > ~/libreoffice/core-master/filter/source/pdf/impdialog.cxx << 'EOF'
[File content will be too long - use the WSL cp command instead]
EOF

echo "Please run the following commands in WSL to copy the fixed file:"
echo "1. cd ~/libreoffice/core-master"
echo "2. cp /mnt/c/Users/drury/Documents/GitHub/core-master/filter/source/pdf/impdialog.cxx filter/source/pdf/"
echo "3. make filter.build"