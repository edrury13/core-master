# PowerShell script to copy all Whisper-related files to WSL Linux location
# This script copies all the files we created or modified for the OpenAI Whisper speech-to-text feature

$sourceBase = "C:\Users\drury\Documents\GitHub\core-master"
$destBase = "\\wsl$\Ubuntu\home\drury\libreoffice\core-master"

# List of all files to copy (created or modified for Whisper feature)
$files = @(
    # Documentation
    "OPENAI_WHISPER_SETUP.md",
    
    # Header files
    "sw\inc\whisper\WhisperManager.hxx",
    "sw\inc\whisper\WhisperConfig.hxx",
    "sw\inc\whisper\AudioCapture.hxx",
    "sw\inc\whisper\WhisperSession.hxx",
    "sw\inc\whisper\WhisperSettingsDialog.hxx",
    
    # Source files
    "sw\source\core\whisper\WhisperManager.cxx",
    "sw\source\core\whisper\WhisperConfig.cxx",
    "sw\source\core\whisper\AudioCapture.cxx",
    "sw\source\core\whisper\WhisperSession.cxx",
    "sw\source\ui\dialog\WhisperSettingsDialog.cxx",
    
    # UI files
    "sw\uiconfig\swriter\ui\whispersettingsdialog.ui",
    
    # Modified files
    "sw\inc\cmdid.h",
    "sw\inc\strings.hrc",
    "sw\source\uibase\inc\textsh.hxx",
    "sw\source\uibase\shells\textsh1.cxx",
    "sw\sdi\whisper.sdi",
    "sw\sdi\swriter.sdi",
    "sw\sdi\swslots.sdi",
    "sw\sdi\textsh.sdi",
    "sw\Library_sw.mk",
    "sw\UIConfig_swriter.mk",
    "sw\uiconfig\swriter\menubar\menubar.xml",
    "sw\uiconfig\swriter\toolbar\standardbar.xml",
    "sw\uiconfig\swriter\toolbar\insertbar.xml",
    "officecfg\registry\data\org\openoffice\Office\UI\GenericCommands.xcu"
)

Write-Host "Starting copy of Whisper files to Linux build location..." -ForegroundColor Green

# Create necessary directories first
$directories = @(
    "sw\inc\whisper",
    "sw\source\core\whisper",
    "sw\source\ui\dialog",
    "sw\uiconfig\swriter\ui",
    "sw\sdi"
)

foreach ($dir in $directories) {
    $linuxDir = $dir -replace '\\', '/'
    $destDir = "$destBase\$linuxDir"
    if (!(Test-Path $destDir)) {
        Write-Host "Creating directory: $destDir" -ForegroundColor Yellow
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
}

# Copy each file
$successCount = 0
$errorCount = 0

foreach ($file in $files) {
    $sourcePath = Join-Path $sourceBase $file
    $linuxPath = $file -replace '\\', '/'
    $destPath = Join-Path $destBase $linuxPath
    
    try {
        if (Test-Path $sourcePath) {
            Copy-Item -Path $sourcePath -Destination $destPath -Force
            Write-Host "✓ Copied: $file" -ForegroundColor Green
            $successCount++
        } else {
            Write-Host "✗ Source not found: $file" -ForegroundColor Red
            $errorCount++
        }
    } catch {
        Write-Host "✗ Error copying $file : $_" -ForegroundColor Red
        $errorCount++
    }
}

Write-Host "`nCopy complete!" -ForegroundColor Cyan
Write-Host "Successfully copied: $successCount files" -ForegroundColor Green
if ($errorCount -gt 0) {
    Write-Host "Errors encountered: $errorCount files" -ForegroundColor Red
}

Write-Host "`nNext steps in Linux:" -ForegroundColor Yellow
Write-Host "1. cd ~/libreoffice/core-master" -ForegroundColor White
Write-Host "2. make sw.clean" -ForegroundColor White
Write-Host "3. make sw" -ForegroundColor White
Write-Host "4. make" -ForegroundColor White
Write-Host "`nDon't forget to set the OpenAI API key:" -ForegroundColor Yellow
Write-Host 'export LIBREOFFICE_OPENAI_API_KEY="your-api-key-here"' -ForegroundColor White