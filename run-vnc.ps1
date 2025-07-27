# run-vnc.ps1 - Windows PowerShell script to run LibreOffice VNC environment

$IMAGE_NAME = "libreoffice-builder-vnc"
$CONTAINER_NAME = "libreoffice-vnc"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Starting LibreOffice VNC Environment" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# Check if Docker is running
try {
    docker info | Out-Null
} catch {
    Write-Host "Error: Docker is not running or not accessible" -ForegroundColor Red
    exit 1
}

# Check if image exists
try {
    docker image inspect $IMAGE_NAME | Out-Null
} catch {
    Write-Host "Docker image not found. Building it first..." -ForegroundColor Yellow
    & "$PSScriptRoot\build.ps1"
}

# Stop and remove existing container if it exists
$existingContainer = docker ps -a --format "table {{.Names}}" | Select-String -Pattern "^$CONTAINER_NAME$"
if ($existingContainer) {
    Write-Host "Stopping existing container: $CONTAINER_NAME" -ForegroundColor Yellow
    docker stop $CONTAINER_NAME | Out-Null
    docker rm $CONTAINER_NAME | Out-Null
}

# Get current directory path for volume mount (convert to WSL path if needed)
$currentPath = Get-Location
$sourcePath = $currentPath.Path

# Run the container
Write-Host "Starting container: $CONTAINER_NAME" -ForegroundColor Green
docker run -d `
    --name $CONTAINER_NAME `
    -p 5901:5901 `
    -p 6080:6080 `
    -v "${sourcePath}:/core:ro" `
    -v "libreoffice-build:/build" `
    -v "libreoffice-ccache:/ccache" `
    -v "libreoffice-home:/home/builder" `
    -e VNC_RESOLUTION=1920x1080 `
    -e VNC_PASSWORD=libreoffice `
    -e PARALLELISM=4 `
    --restart unless-stopped `
    $IMAGE_NAME

# Wait for services to start
Write-Host "Waiting for services to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 10

# Display connection information
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "LibreOffice VNC Environment Started!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Container: $CONTAINER_NAME" -ForegroundColor White
Write-Host ""
Write-Host "Connection Options:" -ForegroundColor Yellow
Write-Host "1. VNC Client: localhost:5901" -ForegroundColor White
Write-Host "2. Web Browser: http://localhost:6080/vnc.html" -ForegroundColor White
Write-Host "   Password: libreoffice" -ForegroundColor Gray
Write-Host ""
Write-Host "Desktop Environment: XFCE4" -ForegroundColor White
Write-Host "LibreOffice Source: /core (read-only)" -ForegroundColor White
Write-Host "Build Directory: /build" -ForegroundColor White
Write-Host ""
Write-Host "Available desktop shortcuts:" -ForegroundColor Yellow
Write-Host "- Build LibreOffice" -ForegroundColor White
Write-Host "- LibreOffice Writer" -ForegroundColor White
Write-Host "- LibreOffice Calc" -ForegroundColor White
Write-Host ""
Write-Host "To view logs:" -ForegroundColor Yellow
Write-Host "  docker logs -f $CONTAINER_NAME" -ForegroundColor White
Write-Host ""
Write-Host "To stop:" -ForegroundColor Yellow
Write-Host "  docker stop $CONTAINER_NAME" -ForegroundColor White
Write-Host ""

# Try to open the web interface automatically
if (Get-Command "start" -ErrorAction SilentlyContinue) {
    Write-Host "Opening web interface in browser..." -ForegroundColor Green
    Start-Sleep -Seconds 3
    start "http://localhost:6080/vnc.html"
}

Write-Host "============================================" -ForegroundColor Cyan