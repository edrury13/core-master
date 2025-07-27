#!/usr/bin/env pwsh
# Copy all Whisper-related files to WSL

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying Whisper implementation files to WSL..." -ForegroundColor Green

# Copy the menu configuration
$src = Join-Path $WindowsRoot "sw\uiconfig\swriter\menubar\menubar.xml"
$dst = Join-Path $WSLRoot "sw\uiconfig\swriter\menubar\menubar.xml"
Write-Host "Copying menubar.xml..."
Copy-Item -Path $src -Destination $dst -Force

# Copy the command definitions  
$src = Join-Path $WindowsRoot "officecfg\registry\data\org\openoffice\Office\UI\GenericCommands.xcu"
$dst = Join-Path $WSLRoot "officecfg\registry\data\org\openoffice\Office\UI\GenericCommands.xcu"
Write-Host "Copying GenericCommands.xcu..."
Copy-Item -Path $src -Destination $dst -Force

# Copy Writer-specific commands if there are any
$src = Join-Path $WindowsRoot "officecfg\registry\data\org\openoffice\Office\UI\WriterCommands.xcu"
$dst = Join-Path $WSLRoot "officecfg\registry\data\org\openoffice\Office\UI\WriterCommands.xcu"
if (Test-Path $src) {
    Write-Host "Copying WriterCommands.xcu..."
    Copy-Item -Path $src -Destination $dst -Force
}

Write-Host "Done! Now run 'make sw && make officecfg' in WSL to rebuild." -ForegroundColor Green