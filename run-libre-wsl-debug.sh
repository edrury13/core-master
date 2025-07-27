#!/bin/bash
# Run LibreOffice in WSL with debug logging and browser handling

echo "Starting LibreOffice with debug logging in WSL..."
echo "================================================"

# Enable debug logging
export SAL_LOG="+INFO.cui.dialogs+WARN.cui.dialogs+INFO.ucb.gdocs+WARN.ucb.gdocs+INFO.sfx.doc+WARN"
export SAL_LOG_FILE=-

# Set browser to use Windows browser via WSL
export BROWSER="/mnt/c/Windows/System32/cmd.exe /c start"

# Alternative: use wslview if available
if command -v wslview &> /dev/null; then
    export BROWSER="wslview"
fi

cd ~/libreoffice/core-master

echo "Debug logging enabled for:"
echo "- cui.dialogs (GoogleDriveFilePicker)"
echo "- ucb.gdocs (Google Drive auth)"
echo "- sfx.doc (Save operations)"
echo ""
echo "Browser set to: $BROWSER"
echo ""
echo "Starting LibreOffice Writer..."
echo "================================================"
echo ""

# Run LibreOffice
./instdir/program/soffice --writer 2>&1 | tee gdrive-debug.log