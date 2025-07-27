#!/usr/bin/env pwsh
# Script to copy spell check improvements to WSL LibreOffice build directory

$wslPath = "\\wsl$\Ubuntu\home\user\libreoffice\core-master"
$sourcePath = "C:\Users\drury\Documents\GitHub\core-master"

Write-Host "Copying spell check improvements to WSL..." -ForegroundColor Green

# List of files to copy
$files = @(
    # Increased suggestion limit changes
    "editeng\source\editeng\editview.cxx",
    "sw\source\core\edit\edlingu.cxx",
    
    # Learn from Document feature
    "cui\source\dialogs\SpellDialog.cxx",
    "cui\source\inc\SpellDialog.hxx",
    "cui\uiconfig\ui\spellingdialog.ui",
    "cui\uiconfig\ui\learnfromdocdialog.ui",
    "cui\inc\strings.hrc"
)

$successCount = 0
$errorCount = 0

foreach ($file in $files) {
    $source = Join-Path $sourcePath $file
    $destination = Join-Path $wslPath $file.Replace('\', '/')
    
    try {
        # Create destination directory if it doesn't exist
        $destDir = Split-Path $destination -Parent
        if (!(Test-Path $destDir)) {
            New-Item -ItemType Directory -Path $destDir -Force | Out-Null
        }
        
        # Copy the file
        Copy-Item -Path $source -Destination $destination -Force
        Write-Host "✓ Copied: $file" -ForegroundColor Green
        $successCount++
    }
    catch {
        Write-Host "✗ Failed to copy: $file" -ForegroundColor Red
        Write-Host "  Error: $_" -ForegroundColor Red
        $errorCount++
    }
}

Write-Host "`nSummary:" -ForegroundColor Cyan
Write-Host "Successfully copied: $successCount files" -ForegroundColor Green
if ($errorCount -gt 0) {
    Write-Host "Failed to copy: $errorCount files" -ForegroundColor Red
}

Write-Host "`nNext steps in WSL:" -ForegroundColor Yellow
Write-Host "1. cd ~/libreoffice/core-master" -ForegroundColor White
Write-Host "2. make cui.clean && make cui" -ForegroundColor White
Write-Host "3. make editeng.clean && make editeng" -ForegroundColor White
Write-Host "4. make sw.clean && make sw" -ForegroundColor White
Write-Host "5. make check" -ForegroundColor White