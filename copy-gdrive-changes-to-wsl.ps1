#!/usr/bin/env pwsh
# Copy Google Drive changes to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying Google Drive changes to WSL..." -ForegroundColor Green

# Copy modified files
$files = @(
    # CUI files
    "cui\source\dialogs\GoogleDriveFilePicker.cxx",
    "cui\source\dialogs\GoogleDriveFilePicker.hxx",
    
    # SFX2 files
    "sfx2\source\doc\gdrivesync.cxx",
    "include\sfx2\gdrivesync.hxx",
    "sfx2\source\doc\objstor.cxx",
    "sfx2\Library_sfx.mk"
)

foreach ($file in $files) {
    $src = Join-Path $WindowsRoot $file
    $dst = Join-Path $WSLRoot $file.Replace('\', '/')
    
    if (Test-Path $src) {
        Write-Host "Copying $file..."
        Copy-Item -Path $src -Destination $dst -Force
    } else {
        Write-Host "Warning: $src not found!" -ForegroundColor Yellow
    }
}

Write-Host "`nAll files copied successfully!" -ForegroundColor Green
Write-Host "`nNext steps:"
Write-Host "1. Open WSL terminal"
Write-Host "2. cd ~/libreoffice/core-master"
Write-Host "3. make"