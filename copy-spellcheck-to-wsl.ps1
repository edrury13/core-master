# Script to copy spell check improvements to WSL LibreOffice build directory

$wslPath = "\\wsl$\Ubuntu\home\user\libreoffice\core-master"
$sourcePath = "C:\Users\drury\Documents\GitHub\core-master"

Write-Host "Copying spell check improvements to WSL..." -ForegroundColor Green

# List of files to copy
$files = @(
    "editeng\source\editeng\editview.cxx",
    "sw\source\core\edit\edlingu.cxx",
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
        Write-Host "Copied: $file" -ForegroundColor Green
        $successCount++
    }
    catch {
        Write-Host "Failed to copy: $file" -ForegroundColor Red
        Write-Host "  Error: $_" -ForegroundColor Red
        $errorCount++
    }
}

Write-Host ""
Write-Host "Summary:" -ForegroundColor Cyan
Write-Host "Successfully copied: $successCount files" -ForegroundColor Green
if ($errorCount -gt 0) {
    Write-Host "Failed to copy: $errorCount files" -ForegroundColor Red
}

Write-Host ""
Write-Host "Next steps in WSL:" -ForegroundColor Yellow
Write-Host "1. cd ~/libreoffice/core-master"
Write-Host "2. make cui.clean"
Write-Host "3. make cui"
Write-Host "4. make editeng.clean"
Write-Host "5. make editeng"
Write-Host "6. make sw.clean"
Write-Host "7. make sw"
Write-Host "8. make check"