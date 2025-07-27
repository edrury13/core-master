#!/bin/bash
# Copy updated Whisper audio files within WSL

echo "Copying updated Whisper audio files..."

# Copy the files
cp -v /mnt/c/Users/drury/Documents/GitHub/core-master/sw/source/core/whisper/AudioCapture.cxx /home/drury/libreoffice/core-master/sw/source/core/whisper/
cp -v /mnt/c/Users/drury/Documents/GitHub/core-master/sw/source/core/whisper/WhisperManager.cxx /home/drury/libreoffice/core-master/sw/source/core/whisper/
cp -v /mnt/c/Users/drury/Documents/GitHub/core-master/sw/source/core/whisper/WhisperSession.cxx /home/drury/libreoffice/core-master/sw/source/core/whisper/

echo -e "\nFiles copied successfully!"
echo -e "\nTo rebuild, run:"
echo "cd /home/drury/libreoffice/core-master && make sw.build"