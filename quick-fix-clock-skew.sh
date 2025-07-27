#!/bin/bash
# Quick fix for clock skew - just touch our modified files

cd ~/libreoffice/core-master

echo "Touching modified files to fix clock skew..."

# Touch our modified files
touch cui/source/dialogs/GoogleDriveFilePicker.cxx
touch cui/source/dialogs/GoogleDriveFilePicker.hxx
touch cui/source/dialogs/GoogleDriveAuthService.cxx
touch cui/Library_cui.mk
touch sfx2/source/doc/gdrivesync.cxx
touch include/sfx2/gdrivesync.hxx
touch sfx2/source/doc/objstor.cxx
touch sfx2/Library_sfx.mk
touch ucb/Module_ucb.mk
touch include/ucb/gdocsauth.hxx

# Touch the build directories
find workdir/CxxObject/cui -name "*.o" -exec touch {} \;
find workdir/CxxObject/sfx2 -name "*.o" -exec touch {} \;

echo "Done. Now run:"
echo "make cui.build"
echo "make sfx.build"