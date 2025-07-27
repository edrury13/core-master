@echo off
echo Copying updated Whisper audio files to WSL...

wsl cp -v "/mnt/c/Users/drury/Documents/GitHub/core-master/sw/source/core/whisper/AudioCapture.cxx" "/home/drury/libreoffice/core-master/sw/source/core/whisper/AudioCapture.cxx"
wsl cp -v "/mnt/c/Users/drury/Documents/GitHub/core-master/sw/source/core/whisper/WhisperManager.cxx" "/home/drury/libreoffice/core-master/sw/source/core/whisper/WhisperManager.cxx"
wsl cp -v "/mnt/c/Users/drury/Documents/GitHub/core-master/sw/source/core/whisper/WhisperSession.cxx" "/home/drury/libreoffice/core-master/sw/source/core/whisper/WhisperSession.cxx"

echo.
echo Files copied successfully!
echo.
echo To rebuild in WSL, run:
echo cd /home/drury/libreoffice/core-master ^&^& make sw.build