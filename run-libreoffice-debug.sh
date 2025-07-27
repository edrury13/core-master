#!/bin/bash
# Run LibreOffice with Google Drive debug logging enabled

echo "Starting LibreOffice with debug logging..."
echo "========================================="

# Set up environment for maximum logging
export SAL_LOG="+INFO.cui.dialogs+WARN.cui.dialogs+INFO.ucb.gdocs+WARN.ucb.gdocs+INFO.sfx.doc+WARN"

# Also enable console output
export SAL_LOG_FILE=-

# Path to the LibreOffice installation
INSTDIR="$HOME/libreoffice/core-master/instdir/program"

if [ ! -f "$INSTDIR/soffice" ]; then
    echo "Error: LibreOffice not found at $INSTDIR"
    echo "Please make sure you've built and installed LibreOffice"
    exit 1
fi

echo "Debug logging enabled for:"
echo "- cui.dialogs (GoogleDriveFilePicker)"
echo "- ucb.gdocs (Google Drive auth)" 
echo "- sfx.doc (Save operations)"
echo ""
echo "Log output will appear in the console."
echo "Starting LibreOffice Writer..."
echo ""

# Run LibreOffice Writer
"$INSTDIR/soffice" --writer 2>&1 | tee ~/gdrive-debug.log

echo ""
echo "LibreOffice closed. Debug log saved to ~/gdrive-debug.log"