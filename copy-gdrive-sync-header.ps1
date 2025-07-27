#!/usr/bin/env pwsh
# Copy Google Drive sync header to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying Google Drive sync header to WSL..." -ForegroundColor Green

# Copy gdrivesync.hxx
$src = Join-Path $WindowsRoot "include\sfx2\gdrivesync.hxx"
$dst = Join-Path $WSLRoot "include\sfx2\gdrivesync.hxx"

if (Test-Path $src) {
    Write-Host "Copying gdrivesync.hxx..."
    Copy-Item -Path $src -Destination $dst -Force
}

# Also copy the auth service again
$src = Join-Path $WindowsRoot "cui\source\dialogs\GoogleDriveAuthService.cxx"
$dst = Join-Path $WSLRoot "cui\source\dialogs\GoogleDriveAuthService.cxx"

if (Test-Path $src) {
    Write-Host "Copying GoogleDriveAuthService.cxx..."
    Copy-Item -Path $src -Destination $dst -Force
}

Write-Host "Done! Now run 'make sfx2 && make cui' in WSL to rebuild." -ForegroundColor Green