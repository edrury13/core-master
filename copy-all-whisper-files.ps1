#!/usr/bin/env pwsh
# Copy ALL Whisper files to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying ALL Whisper files to WSL..." -ForegroundColor Green

# Create whisper directories if they don't exist
$dirs = @(
    "sw\inc\whisper",
    "sw\source\core\whisper",
    "sw\source\ui\dialog"
)

foreach ($dir in $dirs) {
    $wslDir = Join-Path $WSLRoot $dir
    if (!(Test-Path $wslDir)) {
        Write-Host "Creating directory: $dir"
        New-Item -ItemType Directory -Path $wslDir -Force | Out-Null
    }
}

# Copy header files
$headers = @(
    "sw\inc\whisper\WhisperManager.hxx",
    "sw\inc\whisper\WhisperConfig.hxx",
    "sw\inc\whisper\AudioCapture.hxx",
    "sw\inc\whisper\WhisperSession.hxx",
    "sw\inc\whisper\WhisperSettingsDialog.hxx"
)

foreach ($file in $headers) {
    $src = Join-Path $WindowsRoot $file
    $dst = Join-Path $WSLRoot $file
    if (Test-Path $src) {
        Write-Host "Copying $file..."
        Copy-Item -Path $src -Destination $dst -Force
    }
}

# Copy source files
$sources = @(
    "sw\source\core\whisper\WhisperManager.cxx",
    "sw\source\core\whisper\WhisperConfig.cxx",
    "sw\source\core\whisper\AudioCapture.cxx",
    "sw\source\core\whisper\WhisperSession.cxx",
    "sw\source\ui\dialog\WhisperSettingsDialog.cxx"
)

foreach ($file in $sources) {
    $src = Join-Path $WindowsRoot $file
    $dst = Join-Path $WSLRoot $file
    if (Test-Path $src) {
        Write-Host "Copying $file..."
        Copy-Item -Path $src -Destination $dst -Force
    }
}

# Copy SDI files
$sdiFiles = @(
    "sw\sdi\whisper.sdi",
    "sw\sdi\swriter.sdi",
    "sw\sdi\textsh.sdi"
)

foreach ($file in $sdiFiles) {
    $src = Join-Path $WindowsRoot $file
    $dst = Join-Path $WSLRoot $file
    if (Test-Path $src) {
        Write-Host "Copying $file..."
        Copy-Item -Path $src -Destination $dst -Force
    }
}

# Copy UI files
$uiFiles = @(
    "sw\uiconfig\swriter\ui\whispersettingsdialog.ui"
)

foreach ($file in $uiFiles) {
    $src = Join-Path $WindowsRoot $file
    $dst = Join-Path $WSLRoot $file
    if (Test-Path $src) {
        Write-Host "Copying $file..."
        Copy-Item -Path $src -Destination $dst -Force
    }
}

# Copy other modified files
$modifiedFiles = @(
    "sw\source\uibase\shells\textsh1.cxx",
    "sw\source\uibase\inc\textsh.hxx",
    "sw\inc\cmdid.h",
    "sw\inc\strings.hrc",
    "sw\Library_sw.mk",
    "sw\UIConfig_swriter.mk"
)

foreach ($file in $modifiedFiles) {
    $src = Join-Path $WindowsRoot $file
    $dst = Join-Path $WSLRoot $file
    if (Test-Path $src) {
        Write-Host "Copying $file..."
        Copy-Item -Path $src -Destination $dst -Force
    }
}

Write-Host "Done! Now run 'make sw' in WSL to rebuild." -ForegroundColor Green