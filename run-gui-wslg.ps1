# Simple WSLg launcher for LibreOffice
param(
    [string]$App = "soffice"
)

$container = "libreoffice-dev"

# Check container
if ((docker ps --filter "name=$container" --format "{{.Names}}") -ne $container) {
    Write-Host "Starting container..." -ForegroundColor Yellow
    docker start $container
    Start-Sleep -Seconds 3
}

Write-Host "Launching LibreOffice with WSLg..." -ForegroundColor Green

# Run with WSLg - it's already configured in docker-compose
docker exec -it $container bash -c "/build/instdir/program/$App"