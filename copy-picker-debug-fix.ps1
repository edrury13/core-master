#!/usr/bin/env pwsh
# Copy updated GoogleDriveFilePicker to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying updated GoogleDriveFilePicker.cxx to WSL..." -ForegroundColor Green

$src = Join-Path $WindowsRoot "cui\source\dialogs\GoogleDriveFilePicker.cxx"
$dst = Join-Path $WSLRoot "cui\source\dialogs\GoogleDriveFilePicker.cxx"

if (Test-Path $src) {
    Write-Host "Copying GoogleDriveFilePicker.cxx..."
    Copy-Item -Path $src -Destination $dst -Force
}

Write-Host "Done! Now run 'make cui' in WSL to rebuild." -ForegroundColor Green