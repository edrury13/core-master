# Script to copy modified files to WSL
$wslPath = "\\wsl$\Ubuntu\home\drury\libreoffice\core-master"

# List of files to copy
$files = @(
    "sw\source\uibase\uiview\view2.cxx",
    "sd\sdi\_drvwsh.sdi",
    "sd\source\ui\view\drviews3.cxx",
    "sd\source\ui\view\drviewsa.cxx"
)

foreach ($file in $files) {
    $source = "C:\Users\drury\Documents\GitHub\core-master\$file"
    $destination = "$wslPath\$file"
    
    # Ensure destination directory exists
    $destDir = Split-Path -Parent $destination
    if (-not (Test-Path $destDir)) {
        Write-Host "Creating directory: $destDir"
        New-Item -Path $destDir -ItemType Directory -Force | Out-Null
    }
    
    # Copy the file
    Write-Host "Copying $file..."
    Copy-Item -Path $source -Destination $destination -Force
    Write-Host "Copied successfully!"
}

Write-Host "All files copied to WSL!"