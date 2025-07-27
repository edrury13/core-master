# Monitor setup and run LibreOffice when ready
Write-Host "Monitoring LibreOffice setup..." -ForegroundColor Green

$maxWait = 600  # 10 minutes
$waited = 0

# Wait for package installation
Write-Host "`nWaiting for package installation to complete..." -ForegroundColor Yellow
while ($waited -lt $maxWait) {
    $aptRunning = docker exec libreoffice-dev bash -c "pgrep apt-get" 2>$null
    if (-not $aptRunning) {
        Write-Host "`nPackage installation complete!" -ForegroundColor Green
        break
    }
    Write-Host "." -NoNewline
    Start-Sleep -Seconds 10
    $waited += 10
}

# Install minimal X11 tools if needed
Write-Host "`nEnsuring X11 tools are installed..." -ForegroundColor Yellow
docker exec libreoffice-dev bash -c "apt-get install -y x11-apps libxml2-utils 2>/dev/null || echo 'Already installed or locked'"

# Test X11
Write-Host "`nTesting X11 connection..." -ForegroundColor Yellow
$x11Test = docker exec libreoffice-dev bash -c "DISPLAY=172.28.160.1:0.0 timeout 2 xclock 2>&1"
if ($x11Test -notlike "*command not found*" -and $x11Test -notlike "*cannot open display*") {
    Write-Host "X11 connection successful!" -ForegroundColor Green
} else {
    Write-Host "X11 test failed: $x11Test" -ForegroundColor Red
}

# Check for build
Write-Host "`nChecking for LibreOffice build..." -ForegroundColor Yellow
$buildExists = docker exec libreoffice-dev bash -c "test -f /build/instdir/program/soffice && echo 'yes' || echo 'no'" 2>$null

if ($buildExists -eq "yes") {
    Write-Host "LibreOffice build found!" -ForegroundColor Green
    Write-Host "`nRunning LibreOffice Writer..." -ForegroundColor Yellow
    docker exec -d libreoffice-dev bash -c "DISPLAY=172.28.160.1:0.0 /build/instdir/program/soffice --writer"
    Write-Host "LibreOffice Writer should be opening on your screen!" -ForegroundColor Green
} else {
    Write-Host "No build found." -ForegroundColor Red
    Write-Host "`nTo build LibreOffice (takes 1-2 hours):" -ForegroundColor Yellow
    Write-Host "1. Run: docker exec -it libreoffice-dev bash" -ForegroundColor Gray
    Write-Host "2. Run: cd /build && ./autogen.sh && make -j4" -ForegroundColor Gray
    
    # Start the build automatically
    $response = Read-Host "`nStart building LibreOffice now? (y/n)"
    if ($response -eq 'y') {
        Write-Host "`nStarting LibreOffice build..." -ForegroundColor Green
        Write-Host "This will take 1-2 hours. You can monitor progress with:" -ForegroundColor Yellow
        Write-Host "docker logs -f libreoffice-dev" -ForegroundColor Gray
        
        docker exec -d libreoffice-dev bash -c "cd /build && ./autogen.sh && make -j4 && echo 'BUILD COMPLETE!' > /build/.build_complete"
    }
}