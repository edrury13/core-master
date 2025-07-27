# PowerShell script to copy PDF measurement feature changes to WSL

$sourceBase = "C:\Users\drury\Documents\GitHub\core-master"
$wslPath = "\\wsl$\Ubuntu\home\drury\libreoffice\core-master"

Write-Host "Copying PDF Measurement Tools feature files to WSL..." -ForegroundColor Green

# List of modified files
$files = @(
    # Dialog implementation
    "filter\source\pdf\impdialog.hxx",
    "filter\source\pdf\impdialog.cxx",
    "filter\uiconfig\ui\pdfgeneralpage.ui",
    
    # PDF export implementation
    "filter\source\pdf\pdfexport.hxx",
    "filter\source\pdf\pdfexport.cxx",
    
    # PDF writer implementation
    "include\vcl\pdfwriter.hxx",
    "vcl\source\gdi\pdfwriter_impl.cxx"
)

$successCount = 0
$failCount = 0

foreach ($file in $files) {
    $source = Join-Path $sourceBase $file
    $destination = Join-Path $wslPath $file.Replace('\', '/')
    
    # Create destination directory if it doesn't exist
    $destDir = Split-Path $destination -Parent
    if (!(Test-Path $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
    }
    
    try {
        Copy-Item -Path $source -Destination $destination -Force
        Write-Host "OK Copied: $file" -ForegroundColor Green
        $successCount++
    }
    catch {
        Write-Host "X Failed to copy: $file" -ForegroundColor Red
        Write-Host "  Error: $_" -ForegroundColor Yellow
        $failCount++
    }
}

Write-Host "`nSummary:" -ForegroundColor Cyan
Write-Host "Successfully copied: $successCount files" -ForegroundColor Green
if ($failCount -gt 0) {
    Write-Host "Failed to copy: $failCount files" -ForegroundColor Red
}

Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "1. Open WSL and navigate to ~/libreoffice/core-master"
Write-Host "2. Run: make filter.build"
Write-Host "3. Run: make vcl.build"
Write-Host "4. Run: make sd.build"
Write-Host "5. Test the PDF export with measurement tools in Draw"