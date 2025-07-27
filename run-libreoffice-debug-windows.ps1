#!/usr/bin/env pwsh
# Run LibreOffice with Google Drive debug logging enabled on Windows

Write-Host "Starting LibreOffice with debug logging..." -ForegroundColor Green
Write-Host "========================================="

# Set up environment for maximum logging
$env:SAL_LOG = "+INFO.cui.dialogs+WARN.cui.dialogs+INFO.ucb.gdocs+WARN.ucb.gdocs+INFO.sfx.doc+WARN"

# Enable console output
$env:SAL_LOG_FILE = "-"

# Common LibreOffice installation paths on Windows
$possiblePaths = @(
    "C:\Program Files\LibreOffice\program\soffice.exe",
    "C:\Program Files (x86)\LibreOffice\program\soffice.exe",
    "$env:ProgramFiles\LibreOffice\program\soffice.exe",
    "${env:ProgramFiles(x86)}\LibreOffice\program\soffice.exe"
)

$soffice = $null
foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $soffice = $path
        break
    }
}

if (-not $soffice) {
    # Try to find it in the build directory
    $buildPath = "C:\Users\drury\Documents\GitHub\core-master\instdir\program\soffice.exe"
    if (Test-Path $buildPath) {
        $soffice = $buildPath
    } else {
        Write-Host "Error: LibreOffice not found. Please specify the path." -ForegroundColor Red
        exit 1
    }
}

Write-Host "Found LibreOffice at: $soffice" -ForegroundColor Yellow
Write-Host ""
Write-Host "Debug logging enabled for:" -ForegroundColor Cyan
Write-Host "- cui.dialogs (GoogleDriveFilePicker)"
Write-Host "- ucb.gdocs (Google Drive auth)" 
Write-Host "- sfx.doc (Save operations)"
Write-Host ""
Write-Host "Log output will appear in this console window." -ForegroundColor Yellow
Write-Host "Starting LibreOffice Writer..."
Write-Host ""

# Run LibreOffice Writer
& $soffice --writer 2>&1 | Tee-Object -FilePath "$HOME\gdrive-debug.log"

Write-Host ""
Write-Host "LibreOffice closed. Debug log saved to $HOME\gdrive-debug.log" -ForegroundColor Green