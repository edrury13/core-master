# PowerShell script to manage LibreOffice Docker build
param(
    [switch]$Clean,
    [switch]$FullBuild,
    [switch]$RunAfterBuild
)

$containerName = "libreoffice-dev"
$imageName = "libreoffice-builder"
$sourcePath = $PSScriptRoot

Write-Host "LibreOffice Docker Build Script" -ForegroundColor Cyan
Write-Host "===============================" -ForegroundColor Cyan

# Function to check if container exists
function Test-ContainerExists {
    $result = docker ps -a --filter "name=$containerName" --format "{{.Names}}" 2>$null
    return $result -eq $containerName
}

# Function to check if container is running
function Test-ContainerRunning {
    $result = docker ps --filter "name=$containerName" --format "{{.Names}}" 2>$null
    return $result -eq $containerName
}

# Check if container exists
if (Test-ContainerExists) {
    if (Test-ContainerRunning) {
        Write-Host "Container '$containerName' is already running." -ForegroundColor Green
    } else {
        Write-Host "Starting existing container '$containerName'..." -ForegroundColor Yellow
        docker start $containerName
        Start-Sleep -Seconds 2
    }
} else {
    Write-Host "Container '$containerName' not found. Creating from docker-compose..." -ForegroundColor Yellow
    docker-compose up -d
    Start-Sleep -Seconds 5
}

# Wait for container to be ready
Write-Host "Waiting for container to be ready..." -ForegroundColor Yellow
$maxAttempts = 30
$attempt = 0
while ($attempt -lt $maxAttempts) {
    $status = docker exec $containerName bash -c "echo 'ready'" 2>$null
    if ($status -eq "ready") {
        Write-Host "Container is ready!" -ForegroundColor Green
        break
    }
    $attempt++
    Start-Sleep -Seconds 1
}

if ($attempt -eq $maxAttempts) {
    Write-Host "Container failed to become ready. Check docker logs." -ForegroundColor Red
    exit 1
}

# Sync source code
Write-Host "`nSyncing source code to container..." -ForegroundColor Yellow
$excludes = @(
    "--exclude=.git",
    "--exclude=workdir",
    "--exclude=instdir",
    "--exclude=autom4te.cache",
    "--exclude=*.o",
    "--exclude=*.lo",
    "--exclude=*.la",
    "--exclude=.deps",
    "--exclude=.libs"
)

$rsyncCmd = "rsync -av --delete $($excludes -join ' ') /source/ /build/"
docker exec $containerName bash -c $rsyncCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to sync source code!" -ForegroundColor Red
    exit 1
}

# Build LibreOffice
if ($Clean) {
    Write-Host "`nPerforming clean build..." -ForegroundColor Yellow
    docker exec $containerName bash -c "cd /build && make clean" 2>$null
}

if ($FullBuild -or !(docker exec $containerName bash -c "test -f /build/instdir/program/soffice && echo 'exists'" 2>$null)) {
    Write-Host "`nRunning full build (this will take a while)..." -ForegroundColor Yellow
    docker exec $containerName bash -c "cd /build && ./autogen.sh --without-java --without-help --without-myspell-dicts --with-parallelism=4 && make -j4"
} else {
    Write-Host "`nRunning incremental build..." -ForegroundColor Yellow
    docker exec $containerName bash -c "cd /build && make -j4"
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed! Check the logs with: docker logs $containerName" -ForegroundColor Red
    exit 1
}

Write-Host "`nBuild completed successfully!" -ForegroundColor Green
Write-Host "LibreOffice binary location: /build/instdir/program/soffice" -ForegroundColor Cyan

if ($RunAfterBuild) {
    Write-Host "`nStarting LibreOffice..." -ForegroundColor Yellow
    & "$PSScriptRoot\run-libreoffice-gui.ps1"
}

Write-Host "`nUseful commands:" -ForegroundColor Cyan
Write-Host "  Check logs:        docker logs -f $containerName" -ForegroundColor Gray
Write-Host "  Enter container:   docker exec -it $containerName bash" -ForegroundColor Gray
Write-Host "  Run LibreOffice:   .\run-libreoffice-gui.ps1" -ForegroundColor Gray