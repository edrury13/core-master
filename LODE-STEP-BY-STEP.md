# LODE (LibreOffice Development Environment) Step-by-Step Guide

LODE automates the setup of a LibreOffice build environment on Windows, macOS, and Linux. This guide focuses on Windows setup.

## Prerequisites

- Windows 10 or later
- At least 50 GB free disk space
- Administrator privileges for installation
- Internet connection for downloading dependencies

## Step 1: Download LODE

1. Visit https://wiki.documentfoundation.org/Development/lode
2. Click on the Windows download link
3. Save the file (typically `lode-win.zip` or similar)

## Step 2: Extract LODE

1. Create a directory for LODE (recommended: `C:\lode`)
   ```cmd
   mkdir C:\lode
   ```

2. Extract the downloaded ZIP file to this directory
   - Right-click the ZIP file
   - Select "Extract All..."
   - Choose `C:\lode` as the destination

## Step 3: Run LODE Setup

1. Open Command Prompt as Administrator
   - Press Win+X
   - Select "Windows Terminal (Admin)" or "Command Prompt (Admin)"

2. Navigate to LODE directory
   ```cmd
   cd C:\lode
   ```

3. Run the setup script
   ```cmd
   setup.bat
   ```

## Step 4: LODE Installation Process

The setup script will automatically:

1. **Check system requirements**
   - Verifies Windows version
   - Checks available disk space

2. **Install Cygwin**
   - Downloads Cygwin installer
   - Installs with all required packages
   - Sets up Cygwin environment

3. **Install Visual Studio Build Tools**
   - Downloads VS Build Tools installer
   - Installs C++ compiler and Windows SDK
   - Configures MSVC paths

4. **Install Additional Tools**
   - Java JDK
   - Apache Ant
   - NASM assembler
   - Other build dependencies

5. **Clone LibreOffice Source**
   - Creates `C:\lode\core` directory
   - Clones the LibreOffice git repository

## Step 5: Configure Your Build

1. Open LODE terminal
   ```cmd
   C:\lode\start-shell.bat
   ```

2. Navigate to the core directory
   ```bash
   cd /cygdrive/c/lode/core
   ```

3. Run autogen with your preferred options
   ```bash
   # For a standard build
   ./autogen.sh

   # For a debug build
   ./autogen.sh --enable-debug --enable-dbgutil

   # For a faster build (no Java, no help)
   ./autogen.sh --without-java --without-help --disable-odk
   ```

## Step 6: Build LibreOffice

1. Start the build
   ```bash
   make
   ```

2. For faster builds, use parallel jobs
   ```bash
   make -j8  # Use 8 parallel jobs
   ```

## Step 7: Run Your Build

1. After successful build, run LibreOffice
   ```bash
   instdir/program/soffice.exe
   ```

## LODE Directory Structure

After setup, your LODE directory will contain:
```
C:\lode\
├── bin\          # LODE scripts and utilities
├── core\         # LibreOffice source code
├── cygwin\       # Cygwin installation
├── ext_tar\      # Downloaded external dependencies
├── jenkins\      # Jenkins configuration (optional)
├── logs\         # Build and setup logs
└── opt\          # Additional tools (Java, Ant, etc.)
```

## Useful LODE Commands

### Update Everything
```bash
C:\lode\update-all.bat
```

### Update Only LibreOffice Source
```bash
cd /cygdrive/c/lode/core
git pull --rebase
```

### Clean Build
```bash
cd /cygdrive/c/lode/core
make clean
make
```

### Create Installer
```bash
make win-x86_64-msi
```

## Troubleshooting

### Setup Fails
1. Check internet connection
2. Ensure you have administrator privileges
3. Check `C:\lode\logs\setup.log` for errors

### Build Fails
1. Check available disk space (need ~30GB during build)
2. Update LODE: `C:\lode\update-all.bat`
3. Clean and rebuild: `make clean && make`

### Cygwin Path Issues
LODE sets up paths automatically. If you have issues:
```bash
# In LODE terminal
echo $PATH
which make
which cl.exe
```

### Visual Studio Not Found
1. Re-run `C:\lode\setup.bat`
2. Check VS installation in `C:\lode\logs\vs-install.log`

## Advanced Configuration

### Custom Build Location
Edit `C:\lode\bin\config.ini`:
```ini
[paths]
core_dir=C:\my-custom-path\core
```

### Multiple Branches
```bash
cd /cygdrive/c/lode
git clone https://gerrit.libreoffice.org/core core-master
git clone https://gerrit.libreoffice.org/core core-7.6
```

### Jenkins Integration
LODE can set up Jenkins for continuous integration:
```cmd
C:\lode\setup-jenkins.bat
```

## Tips for Faster Builds

1. **Use ccache**
   ```bash
   ./autogen.sh --enable-ccache
   ```

2. **Disable unnecessary modules**
   ```bash
   ./autogen.sh \
       --without-java \
       --without-help \
       --disable-odk \
       --disable-postgresql-sdbc
   ```

3. **Use RAM disk for temp files**
   - Create RAM disk
   - Set `TMP` and `TEMP` environment variables

4. **Incremental builds**
   ```bash
   make module.build  # Build only changed module
   ```

## Next Steps

1. Read the developer guide: https://wiki.documentfoundation.org/Development
2. Join the mailing list: libreoffice@lists.freedesktop.org
3. Set up IDE integration (VS Code, Visual Studio, etc.)
4. Start with easy hacks: https://wiki.documentfoundation.org/Development/EasyHacks