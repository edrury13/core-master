#!/usr/bin/env pwsh
# Copy updated Whisper audio files to WSL

$sourceDir = "C:\Users\drury\Documents\GitHub\core-master"
$destDir = "/home/drury/libreoffice/core-master"

Write-Host "Copying updated Whisper audio files to WSL..." -ForegroundColor Green

# Copy the updated files
$files = @(
    "sw\source\core\whisper\AudioCapture.cxx",
    "sw\source\core\whisper\WhisperManager.cxx", 
    "sw\source\core\whisper\WhisperSession.cxx"
)

foreach ($file in $files) {
    $srcFile = Join-Path $sourceDir $file
    $destFile = $destDir + "/" + $file.Replace('\', '/')
    
    Write-Host "Copying $file..." -ForegroundColor Yellow
    # Convert Windows path to WSL path
    $wslSrcPath = wsl wslpath -a "$srcFile"
    wsl cp -v "$wslSrcPath" "$destFile"
}

Write-Host "`nFiles copied successfully!" -ForegroundColor Green
Write-Host "`nTo rebuild in WSL, run:" -ForegroundColor Cyan
Write-Host "cd /home/drury/libreoffice/core-master && make sw.build" -ForegroundColor White