#!/bin/bash
# Script to copy modified files to WSL build location

echo "Copying modified files to WSL build location..."

# Base paths
WINDOWS_BASE="/mnt/c/Users/drury/Documents/GitHub/core-master"
WSL_BASE="$HOME/libreoffice/core-master"

# Copy GoogleDriveFilePicker files
echo "Copying Google Drive UI files..."
cp -v "$WINDOWS_BASE/cui/source/dialogs/GoogleDriveFilePicker.cxx" "$WSL_BASE/cui/source/dialogs/"
cp -v "$WINDOWS_BASE/cui/source/dialogs/GoogleDriveFilePicker.hxx" "$WSL_BASE/cui/source/dialogs/"
cp -v "$WINDOWS_BASE/cui/source/dialogs/GoogleDriveFilePickerService.cxx" "$WSL_BASE/cui/source/dialogs/"
cp -v "$WINDOWS_BASE/cui/source/dialogs/GoogleDriveFilePickerService.hxx" "$WSL_BASE/cui/source/dialogs/"

# Copy gdocs UCP files
echo "Copying Google Drive UCP files..."
cp -v "$WINDOWS_BASE/ucb/source/ucp/gdocs/gdocs_auth.cxx" "$WSL_BASE/ucb/source/ucp/gdocs/"
cp -v "$WINDOWS_BASE/ucb/source/ucp/gdocs/gdocs_datasupplier.cxx" "$WSL_BASE/ucb/source/ucp/gdocs/"
cp -v "$WINDOWS_BASE/ucb/source/ucp/gdocs/gdocs_content.cxx" "$WSL_BASE/ucb/source/ucp/gdocs/"
cp -v "$WINDOWS_BASE/ucb/source/ucp/gdocs/gdocs_provider.cxx" "$WSL_BASE/ucb/source/ucp/gdocs/"

# Copy timer files
echo "Copying timer files..."
cp -v "$WINDOWS_BASE/sw/source/uibase/inc/timerctrl.hxx" "$WSL_BASE/sw/source/uibase/inc/"
cp -v "$WINDOWS_BASE/sw/source/uibase/utlui/timerctrl.cxx" "$WSL_BASE/sw/source/uibase/utlui/"
cp -v "$WINDOWS_BASE/sw/source/uibase/uiview/view2.cxx" "$WSL_BASE/sw/source/uibase/uiview/"
cp -v "$WINDOWS_BASE/sfx2/source/view/DocumentTimer.cxx" "$WSL_BASE/sfx2/source/view/"
cp -v "$WINDOWS_BASE/sc/source/ui/cctrl/timerctrl.cxx" "$WSL_BASE/sc/source/ui/cctrl/"
cp -v "$WINDOWS_BASE/sc/source/ui/app/scdll.cxx" "$WSL_BASE/sc/source/ui/app/"
cp -v "$WINDOWS_BASE/sd/source/ui/app/timerctrl.cxx" "$WSL_BASE/sd/source/ui/app/"
cp -v "$WINDOWS_BASE/sd/source/ui/app/sddll.cxx" "$WSL_BASE/sd/source/ui/app/"

echo ""
echo "Verifying GoogleDriveFilePicker has debug changes..."
grep -n "GoogleDriveFilePicker CONSTRUCTOR" "$WSL_BASE/cui/source/dialogs/GoogleDriveFilePicker.cxx"

echo ""
echo "Done! Now run:"
echo "  cd $WSL_BASE"
echo "  rm -rf workdir/CxxObject/cui/source/dialogs/GoogleDriveFilePicker.*"
echo "  make cui"