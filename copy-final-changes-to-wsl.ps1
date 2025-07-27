#!/usr/bin/env pwsh
# Copy all final changes to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying all final changes to WSL..." -ForegroundColor Green

# Copy modified files
$files = @(
    # CUI files
    "cui\source\dialogs\GoogleDriveFilePicker.cxx",
    "cui\source\dialogs\GoogleDriveFilePicker.hxx",
    "cui\source\dialogs\GoogleDriveAuthService.cxx",
    "cui\Library_cui.mk",
    
    # SFX2 files
    "sfx2\source\doc\gdrivesync.cxx",
    "include\sfx2\gdrivesync.hxx",
    "sfx2\source\doc\objstor.cxx",
    "sfx2\Library_sfx.mk",
    
    # UCB files (disabled)
    "ucb\Module_ucb.mk",
    
    # Include files
    "include\ucb\gdocsauth.hxx"
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
Write-Host "3. make cui.build"
Write-Host "4. make sfx.build"