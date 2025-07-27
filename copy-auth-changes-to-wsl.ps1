# Script to copy Google Drive authentication changes to WSL
# This copies all the files we modified for the unified authentication service

$wslPath = "\\wsl$\Ubuntu\home\drury\libreoffice\core-master"
$sourcePath = "C:\Users\drury\Documents\GitHub\core-master"

Write-Host "Copying Google Drive Unified Authentication changes to WSL..." -ForegroundColor Green

# List of files to copy
$files = @(
    # New files
    "include\ucb\gdocsauth.hxx",
    "ucb\source\ucp\gdocs\gdocs_authservice.cxx",
    
    # Modified files
    "cui\source\dialogs\GoogleDriveFilePicker.cxx",
    "ucb\source\ucp\gdocs\gdocs_auth.cxx",
    "ucb\Library_ucpgdocs1.mk"
)

$successCount = 0
$errorCount = 0

foreach ($file in $files) {
    $source = Join-Path $sourcePath $file
    $destination = Join-Path $wslPath ($file -replace '\\', '/')
    
    # Create destination directory if it doesn't exist
    $destDir = Split-Path $destination -Parent
    if (!(Test-Path $destDir)) {
        Write-Host "Creating directory: $destDir" -ForegroundColor Yellow
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
    
    try {
        if (Test-Path $source) {
            Copy-Item -Path $source -Destination $destination -Force
            Write-Host "✓ Copied: $file" -ForegroundColor Green
            $successCount++
        } else {
            Write-Host "✗ Source file not found: $file" -ForegroundColor Red
            $errorCount++
        }
    } catch {
        Write-Host "✗ Error copying $file : $_" -ForegroundColor Red
        $errorCount++
    }
}

Write-Host "`nSummary:" -ForegroundColor Cyan
Write-Host "  Successfully copied: $successCount files" -ForegroundColor Green
if ($errorCount -gt 0) {
    Write-Host "  Failed to copy: $errorCount files" -ForegroundColor Red
}

Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "1. Open WSL terminal"
Write-Host "2. cd ~/libreoffice/core-master"
Write-Host "3. Run: make ucb.clean; make ucb"
Write-Host "4. Run: make cui.clean; make cui"
Write-Host "5. Run: make"