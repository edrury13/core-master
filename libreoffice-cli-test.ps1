# Test LibreOffice Build Without GUI
Write-Host "Testing LibreOffice Build (CLI Mode)..." -ForegroundColor Green

Write-Host "`nYour LibreOffice build is located at:" -ForegroundColor Yellow
Write-Host "/build/instdir/program/soffice" -ForegroundColor White

Write-Host "`nRunning LibreOffice version check..." -ForegroundColor Cyan
docker exec libreoffice-dev bash -c "cd /build/instdir/program && ./soffice --version"

Write-Host "`nTesting LibreOffice Writer (headless mode)..." -ForegroundColor Cyan
docker exec libreoffice-dev bash -c @"
cd /build/instdir/program
# Create a test document
echo 'Hello from LibreOffice built from source!' > /tmp/test.txt
# Convert to PDF using LibreOffice
./soffice --headless --convert-to pdf --outdir /tmp /tmp/test.txt
ls -la /tmp/test.pdf
"@

Write-Host "`nDevelopment Commands:" -ForegroundColor Green
Write-Host "- Rebuild Writer module: docker exec -it libreoffice-dev bash -c 'cd /build && make sw'" -ForegroundColor Gray
Write-Host "- Rebuild Calc module: docker exec -it libreoffice-dev bash -c 'cd /build && make sc'" -ForegroundColor Gray
Write-Host "- Run tests: docker exec -it libreoffice-dev bash -c 'cd /build && make check'" -ForegroundColor Gray
Write-Host "- Interactive shell: docker exec -it libreoffice-dev bash" -ForegroundColor Gray

Write-Host "`nWhile GUI packages install, you can:" -ForegroundColor Yellow
Write-Host "1. Test your build with headless operations" -ForegroundColor Gray
Write-Host "2. Rebuild specific modules" -ForegroundColor Gray
Write-Host "3. Run unit tests" -ForegroundColor Gray

# Check GUI install progress
Write-Host "`nGUI Installation Progress:" -ForegroundColor Yellow
docker exec libreoffice-dev bash -c "dpkg -l 2>/dev/null | grep -E 'x11vnc|xvfb|novnc|websockify' | grep '^ii' | awk '{print $2}'"