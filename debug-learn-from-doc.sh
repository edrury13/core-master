#!/bin/bash
cd /home/drury/libreoffice/core-master

# Enable core dumps
ulimit -c unlimited

# Run with debugging and logging
export SAL_LOG="+INFO.cui.dialogs+WARN"
export MALLOC_CHECK_=3

echo "Starting LibreOffice with debugging..."
echo "When it crashes, check for core dump in current directory"

# Run in gdb to catch the crash
gdb -ex run -ex "bt full" -ex quit --args ./instdir/program/soffice --writer /home/drury/spellcheck-test.txt 2>&1 | tee crash-debug.log