# PowerShell script to copy form field implementation files to WSL
# Run this from the Windows side

$sourceBase = "C:\Users\drury\Documents\GitHub\core-master"
$wslPath = "\\wsl$\Ubuntu\home\$env:USERNAME\libreoffice\core-master"

Write-Host "Copying PDF Form Fields implementation to WSL..." -ForegroundColor Green

# Create arrays of files to copy
$filesToCopy = @(
    # Form control shape classes
    "sd\inc\formcontrolshape.hxx",
    "sd\source\core\formcontrolshape.cxx",
    
    # UI view implementation
    "sd\source\ui\view\drviews_form.cxx",
    
    # PDF export implementation
    "sd\source\ui\unoidl\unomodel_pdfexport.cxx",
    
    # Documentation
    "PDF_FORM_FIELDS_IMPLEMENTATION.md"
)

$filesToModify = @(
    # Command definitions
    @{
        Path = "include\svx\svxids.hrc"
        Desc = "Form control command IDs"
    },
    
    # SDI command definitions
    @{
        Path = "sd\sdi\sdraw.sdi"
        Desc = "Draw command definitions"
    },
    
    # UI modifications
    @{
        Path = "sd\source\ui\view\drviews7.cxx"
        Desc = "Command handler integration"
    },
    
    # Header modifications
    @{
        Path = "sd\source\ui\inc\DrawViewShell.hxx"
        Desc = "DrawViewShell header update"
    },
    
    # Menu configuration
    @{
        Path = "sd\uiconfig\simpress\menubar\menubar.xml"
        Desc = "Menu integration"
    },
    
    # UI strings
    @{
        Path = "officecfg\registry\data\org\openoffice\Office\UI\GenericCommands.xcu"
        Desc = "UI command strings"
    },
    
    # Build system
    @{
        Path = "sd\Library_sd.mk"
        Desc = "Build system integration"
    }
)

# Copy new files
Write-Host "`nCopying new files:" -ForegroundColor Yellow
foreach ($file in $filesToCopy) {
    $source = Join-Path $sourceBase $file
    $dest = Join-Path $wslPath $file.Replace('\', '/')
    $destDir = Split-Path $dest -Parent
    
    # Create directory if it doesn't exist
    if (-not (Test-Path $destDir)) {
        New-Item -ItemType Directory -Path $destDir -Force | Out-Null
        Write-Host "  Created directory: $destDir" -ForegroundColor DarkGray
    }
    
    if (Test-Path $source) {
        Copy-Item -Path $source -Destination $dest -Force
        Write-Host "  ✓ Copied: $file" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Not found: $file" -ForegroundColor Red
    }
}

# Copy modified files
Write-Host "`nCopying modified files:" -ForegroundColor Yellow
foreach ($fileInfo in $filesToModify) {
    $file = $fileInfo.Path
    $desc = $fileInfo.Desc
    $source = Join-Path $sourceBase $file
    $dest = Join-Path $wslPath $file.Replace('\', '/')
    
    if (Test-Path $source) {
        Copy-Item -Path $source -Destination $dest -Force
        Write-Host "  ✓ Copied: $file ($desc)" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Not found: $file" -ForegroundColor Red
    }
}

Write-Host "`nAll files copied to WSL!" -ForegroundColor Green
Write-Host "`nNext steps in WSL:" -ForegroundColor Cyan
Write-Host "1. cd ~/libreoffice/core-master"
Write-Host "2. make sd.clean"
Write-Host "3. make sd"
Write-Host "4. make"

# Also create a bash script for WSL side verification
$verifyScript = @'
#!/bin/bash
# Verify form fields implementation files

echo "Verifying PDF Form Fields implementation..."
echo

# Check new files
echo "Checking new files:"
for file in \
    "sd/inc/formcontrolshape.hxx" \
    "sd/source/core/formcontrolshape.cxx" \
    "sd/source/ui/view/drviews_form.cxx" \
    "sd/source/ui/unoidl/unomodel_pdfexport.cxx" \
    "PDF_FORM_FIELDS_IMPLEMENTATION.md"
do
    if [ -f "$file" ]; then
        echo "  ✓ Found: $file"
    else
        echo "  ✗ Missing: $file"
    fi
done

echo
echo "Checking modified files:"
# Check if form control IDs are in svxids.hrc
if grep -q "SID_INSERT_PUSHBUTTON" "include/svx/svxids.hrc"; then
    echo "  ✓ Form control IDs added to svxids.hrc"
else
    echo "  ✗ Form control IDs missing from svxids.hrc"
fi

# Check if handlers are in drviews7.cxx
if grep -q "InsertFormControl" "sd/source/ui/view/drviews7.cxx"; then
    echo "  ✓ Form control handlers added to drviews7.cxx"
else
    echo "  ✗ Form control handlers missing from drviews7.cxx"
fi

# Check build system
if grep -q "formcontrolshape" "sd/Library_sd.mk"; then
    echo "  ✓ Build system updated"
else
    echo "  ✗ Build system not updated"
fi

echo
echo "Ready to build with: make sd.clean && make sd"
'@

$verifyScriptPath = Join-Path $sourceBase "verify-form-fields.sh"
$verifyScript | Out-File -FilePath $verifyScriptPath -Encoding UTF8

Write-Host "`nAlso created verify-form-fields.sh for WSL verification" -ForegroundColor Cyan