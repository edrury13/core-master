# Test script to verify Whisper environment variable setup

Write-Host "Testing Whisper Environment Variable Setup" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Check if the environment variable is set in Windows
Write-Host "`nChecking Windows environment:" -ForegroundColor Yellow
$winApiKey = [System.Environment]::GetEnvironmentVariable("LIBREOFFICE_OPENAI_API_KEY")
if ($winApiKey) {
    Write-Host "  LIBREOFFICE_OPENAI_API_KEY is set in Windows (length: $($winApiKey.Length))" -ForegroundColor Green
} else {
    Write-Host "  LIBREOFFICE_OPENAI_API_KEY is NOT set in Windows" -ForegroundColor Red
}

# Check WSL environment
Write-Host "`nChecking WSL environment:" -ForegroundColor Yellow
$wslApiKey = wsl.exe bash -c 'echo $LIBREOFFICE_OPENAI_API_KEY'
if ($wslApiKey -and $wslApiKey.Trim()) {
    Write-Host "  LIBREOFFICE_OPENAI_API_KEY is set in WSL (length: $($wslApiKey.Trim().Length))" -ForegroundColor Green
} else {
    Write-Host "  LIBREOFFICE_OPENAI_API_KEY is NOT set in WSL" -ForegroundColor Red
}

# Show how to set the variable
Write-Host "`nTo set the environment variable:" -ForegroundColor Cyan
Write-Host "  Windows (PowerShell):" -ForegroundColor Yellow
Write-Host '    $env:LIBREOFFICE_OPENAI_API_KEY = "your-api-key-here"' -ForegroundColor White
Write-Host "  WSL:" -ForegroundColor Yellow
Write-Host '    export LIBREOFFICE_OPENAI_API_KEY="your-api-key-here"' -ForegroundColor White

# Test running LibreOffice with the variable
Write-Host "`nTo run LibreOffice with the API key:" -ForegroundColor Cyan
Write-Host '  $env:LIBREOFFICE_OPENAI_API_KEY = "your-api-key-here"; .\run-libreoffice.sh' -ForegroundColor White
Write-Host "  OR in WSL:" -ForegroundColor Yellow
Write-Host '  LIBREOFFICE_OPENAI_API_KEY="your-api-key-here" ./instdir/program/soffice' -ForegroundColor White

# Check for logs
Write-Host "`nChecking for recent Whisper logs:" -ForegroundColor Cyan
$logFile = "gdrive-debug.log"
if (Test-Path $logFile) {
    $whisperLogs = Get-Content $logFile | Select-String "sw.whisper" | Select-Object -Last 10
    if ($whisperLogs) {
        Write-Host "Recent Whisper log entries:" -ForegroundColor Yellow
        $whisperLogs | ForEach-Object { Write-Host "  $_" -ForegroundColor Gray }
    } else {
        Write-Host "  No Whisper logs found in $logFile" -ForegroundColor Red
    }
} else {
    Write-Host "  Log file $logFile not found" -ForegroundColor Red
}