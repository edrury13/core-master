#!/usr/bin/env pwsh
# Copy debug scripts to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying debug scripts to WSL..." -ForegroundColor Green

$scripts = @(
    "run-libreoffice-debug.sh",
    "check-gdrive-logs.sh"
)

foreach ($script in $scripts) {
    $src = Join-Path $WindowsRoot $script
    $dst = Join-Path $WSLRoot $script
    
    if (Test-Path $src) {
        Write-Host "Copying $script..."
        Copy-Item -Path $src -Destination $dst -Force
    }
}

Write-Host "Done!" -ForegroundColor Green