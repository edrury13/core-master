# Simple GUI checker
Write-Host "Checking GUI status..." -ForegroundColor Yellow

# Check if services are running
$services = docker exec libreoffice-dev bash -c "ps aux | grep -E 'Xvfb|x11vnc|websockify' | grep -v grep | grep -v bash | wc -l" 2>$null

if ($services -ge 3) {
    Write-Host "`nGUI IS READY!" -ForegroundColor Green
    Write-Host "==============================" -ForegroundColor Cyan
    Write-Host "Open in your browser:" -ForegroundColor Yellow
    Write-Host "http://localhost:6080/vnc.html" -ForegroundColor White
    Start-Process "http://localhost:6080/vnc.html"
} else {
    Write-Host "Still setting up... Found $services/3 services" -ForegroundColor Yellow
    Write-Host "Checking installation progress..." -ForegroundColor Gray
    docker exec libreoffice-dev bash -c "dpkg -l | grep -E 'x11vnc|xvfb|novnc|websockify' | wc -l"
}