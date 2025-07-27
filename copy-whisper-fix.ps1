#!/usr/bin/env pwsh
# Copy Whisper fix to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying Whisper fix to WSL..." -ForegroundColor Green

$src = Join-Path $WindowsRoot "sw\source\uibase\shells\textsh1.cxx"
$dst = Join-Path $WSLRoot "sw\source\uibase\shells\textsh1.cxx"

if (Test-Path $src) {
    Write-Host "Copying textsh1.cxx..."
    Copy-Item -Path $src -Destination $dst -Force
}

Write-Host "Done! Now run 'make sw' in WSL to rebuild." -ForegroundColor Green