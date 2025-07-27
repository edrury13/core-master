#!/bin/bash
# Check LibreOffice logs for Google Drive debug output

echo "Checking for Google Drive debug logs..."
echo "========================================="

# Common log locations
LOG_DIRS=(
    "$HOME/.config/libreoffice/4/user/temp"
    "/tmp"
    "$HOME/.cache/libreoffice"
    "/var/log"
)

# Search for our specific log messages
echo "Searching for GoogleDriveFilePicker logs..."
echo ""

for dir in "${LOG_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "Checking $dir..."
        grep -r "GoogleDriveFilePicker" "$dir" 2>/dev/null | grep -v "Binary file" | head -20
        grep -r "cui.dialogs" "$dir" 2>/dev/null | grep -v "Binary file" | head -20
        grep -r "ucb.gdocs" "$dir" 2>/dev/null | grep -v "Binary file" | head -20
    fi
done

# Check if SAL_LOG is set
echo ""
echo "Current SAL_LOG setting: $SAL_LOG"
echo ""

# Also check for any recent log files
echo "Recent log files:"
find $HOME/.config/libreoffice -name "*.log" -mtime -1 -ls 2>/dev/null

# Check journal if systemd is available
if command -v journalctl &> /dev/null; then
    echo ""
    echo "Checking systemd journal..."
    journalctl --user -n 100 | grep -i "libreoffice\|gdrive\|google" | tail -20
fi

echo ""
echo "To enable debug logging, run LibreOffice with:"
echo "SAL_LOG='+INFO.cui.dialogs+WARN' soffice"