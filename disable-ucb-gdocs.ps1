#!/usr/bin/env pwsh
# Disable UCB gdocs provider since we're using a different approach

$WindowsRoot = "C:\Users\drury\Documents\GitHub\core-master"
$WSLRoot = "\\wsl.localhost\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Disabling UCB gdocs provider..." -ForegroundColor Green

# Comment out the ucpgdocs1 library from Module_ucb.mk
$moduleFile = Join-Path $WindowsRoot "ucb\Module_ucb.mk"
$content = Get-Content $moduleFile -Raw
$content = $content -replace 'Library_ucpgdocs1 \\', '$(if $(DUMMY_NEVER_TRUE),Library_ucpgdocs1,) \'
Set-Content -Path $moduleFile -Value $content

# Copy to WSL
$wslFile = Join-Path $WSLRoot "ucb/Module_ucb.mk"
Copy-Item -Path $moduleFile -Destination $wslFile -Force

Write-Host "UCB gdocs provider disabled!" -ForegroundColor Green