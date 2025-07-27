# Extract LibreOffice build from Docker to run on host
$containerName = "libreoffice-dev"
$extractPath = ".\libreoffice-extracted"

Write-Host "Extracting LibreOffice build from Docker container..." -ForegroundColor Cyan

# Create extraction directory
if (Test-Path $extractPath) {
    Remove-Item -Recurse -Force $extractPath
}
New-Item -ItemType Directory -Path $extractPath | Out-Null

# Copy the built LibreOffice from container
Write-Host "Copying build artifacts..." -ForegroundColor Yellow
docker cp "${containerName}:/build/instdir" "$extractPath/instdir"

Write-Host "`nExtraction complete!" -ForegroundColor Green
Write-Host "LibreOffice build extracted to: $extractPath\instdir" -ForegroundColor Cyan
Write-Host "`nNote: This is a Linux build and won't run directly on Windows." -ForegroundColor Yellow
Write-Host "Consider using WSL2 or continue with Docker + X11 forwarding." -ForegroundColor Yellow