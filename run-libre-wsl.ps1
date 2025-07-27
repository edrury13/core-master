# Run LibreOffice from Docker using WSL2's built-in X11 support
$containerName = "libreoffice-dev"

Write-Host "Running LibreOffice through WSL2..." -ForegroundColor Cyan

# Check if WSL2 is available
$wslVersion = wsl --version 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "WSL2 is not installed. Please install WSL2 first." -ForegroundColor Red
    exit 1
}

# Run through WSL2 which has better process handling
wsl -e bash -c @"
docker exec libreoffice-dev bash -c '
    export DISPLAY=:0
    export WAYLAND_DISPLAY=wayland-0
    export XDG_RUNTIME_DIR=/mnt/wslg/runtime-dir
    export PULSE_SERVER=/mnt/wslg/PulseServer
    cd /build/instdir/program
    ./soffice --writer --nofirststartwizard
'
"@

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nIf that didn't work, try the VNC approach instead." -ForegroundColor Yellow
}