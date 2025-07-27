#!/usr/bin/env pwsh
# Copy gdrivesync.cxx to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying gdrivesync.cxx to WSL..." -ForegroundColor Green

$src = Join-Path $WindowsRoot "sfx2\source\doc\gdrivesync.cxx"
$dst = Join-Path $WSLRoot "sfx2\source\doc\gdrivesync.cxx"

if (Test-Path $src) {
    Write-Host "Copying gdrivesync.cxx..."
    Copy-Item -Path $src -Destination $dst -Force
}

Write-Host "Done! Now run 'make sfx2' in WSL to rebuild." -ForegroundColor Green