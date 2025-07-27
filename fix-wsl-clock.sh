#!/bin/bash
# Fix WSL clock skew issue

echo "Fixing WSL clock skew..."

# Method 1: Force sync with hardware clock
sudo hwclock -s

# Method 2: If that doesn't work, use ntpdate
if command -v ntpdate &> /dev/null; then
    sudo ntpdate -s time.nist.gov
else
    echo "Installing ntpdate..."
    sudo apt-get update
    sudo apt-get install -y ntpdate
    sudo ntpdate -s time.nist.gov
fi

# Method 3: Touch all files to update timestamps
echo "Updating file timestamps..."
cd ~/libreoffice/core-master
find . -name "*.cxx" -o -name "*.hxx" -o -name "*.mk" | xargs touch

echo "Clock sync complete. Current time:"
date

echo ""
echo "Now you can rebuild:"
echo "make clean && make"