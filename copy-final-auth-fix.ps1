#!/usr/bin/env pwsh
# Copy all auth service files to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying Google Drive auth service files to WSL..." -ForegroundColor Green

# Copy GoogleDriveAuthService.cxx
$src = Join-Path $WindowsRoot "cui\source\dialogs\GoogleDriveAuthService.cxx"
$dst = Join-Path $WSLRoot "cui\source\dialogs\GoogleDriveAuthService.cxx"

if (Test-Path $src) {
    Write-Host "Copying GoogleDriveAuthService.cxx..."
    Copy-Item -Path $src -Destination $dst -Force
} else {
    Write-Host "Source file not found: $src" -ForegroundColor Red
}

Write-Host "Done! Now run 'make cui' in WSL to rebuild." -ForegroundColor Green