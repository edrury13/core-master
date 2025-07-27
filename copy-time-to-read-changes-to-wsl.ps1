# PowerShell script to copy Time to Read feature changes to WSL

$sourceBase = "C:\Users\drury\Documents\GitHub\core-master"
$wslPath = "\\wsl$\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying Time to Read feature files to WSL..." -ForegroundColor Green

# List of modified files
$files = @(
    # Document Statistics Panel files
    "sw\source\uibase\sidebar\DocumentStatisticsPanel.hxx",
    "sw\source\uibase\sidebar\DocumentStatisticsPanel.cxx",
    "sw\uiconfig\swriter\ui\documentstatisticspanel.ui"
)

$successCount = 0
$failCount = 0

foreach ($file in $files) {
    $source = Join-Path $sourceBase $file
    $destination = Join-Path $wslPath $file.Replace('\', '/')
    
    # Create destination directory if it doesn't exist
    $destDir = Split-Path $destination -Parent
    if (!(Test-Path $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
    
    try {
        Copy-Item -Path $source -Destination $destination -Force
        Write-Host "✓ Copied: $file" -ForegroundColor Green
        $successCount++
    }
    catch {
        Write-Host "✗ Failed to copy: $file" -ForegroundColor Red
        Write-Host "  Error: $_" -ForegroundColor Yellow
        $failCount++
    }
}

Write-Host "`nSummary:" -ForegroundColor Cyan
Write-Host "Successfully copied: $successCount files" -ForegroundColor Green
if ($failCount -gt 0) {
    Write-Host "Failed to copy: $failCount files" -ForegroundColor Red
}

Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "1. Open WSL and navigate to ~/libreoffice/core-master"
Write-Host "2. Run: make sw.build"
Write-Host "3. Test the Time to Read feature in Writer Document Statistics panel"