# LibreOffice Windows Build Guide

This guide provides comprehensive instructions for building LibreOffice on Windows using different approaches.

## Prerequisites

### 1. Install Cygwin 64-bit
Download from https://www.cygwin.com/

Install these packages during setup:
- autoconf
- automake
- bison
- flex
- gcc-g++
- git
- gnupg
- gperf
- make
- nasm
- patch
- perl
- pkg-config
- python3
- python3-devel
- zip
- unzip

### 2. Install Visual Studio 2019 or 2022
- Download Community Edition (free) from https://visualstudio.microsoft.com/
- During installation, select "Desktop development with C++"
- Ensure Windows 10 SDK is included

### 3. Install Additional Tools
- Java Development Kit (JDK) 17 or later
- Apache Ant (for Java builds)

## Option 1: Native Windows Build with Cygwin

### 1. Open Cygwin Terminal

### 2. Set Environment Variables
```bash
# Adjust paths based on your VS installation
export INCLUDE="C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/include"
export LIB="C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/lib/x64"
```

### 3. Configure the Build
```bash
# For 64-bit build
./autogen.sh \
    --with-visual-studio=2019 \
    --with-windows-sdk=10.0.19041.0 \
    --enable-64-bit \
    --with-distro=LibreOfficeWin64

# For faster debug build
./autogen.sh \
    --with-visual-studio=2019 \
    --with-windows-sdk=10.0.19041.0 \
    --enable-debug \
    --enable-dbgutil \
    --without-java \
    --without-help \
    --disable-odk
```

### 4. Build
```bash
make
```

## Option 2: WSL2 Build (Recommended)

Since you're already using WSL, this might be the easiest approach.

### 1. Fix Clock Skew Issue
```bash
# Sync hardware clock
sudo hwclock -s

# Alternative: touch all files
find . -type f -exec touch {} +
```

### 2. Install Build Dependencies in WSL
```bash
# Update package list
sudo apt-get update

# Install build dependencies
sudo apt-get install -y \
    build-essential \
    git \
    autoconf \
    automake \
    libtool \
    pkg-config \
    python3-dev \
    libxml2-dev \
    libxslt1-dev \
    libcurl4-openssl-dev \
    libboost-dev \
    gperf \
    nasm \
    bison \
    flex \
    zip \
    unzip
```

### 3. Cross-Compile for Windows
```bash
# Install MinGW cross-compiler
sudo apt-get install -y mingw-w64

# Configure for Windows cross-compilation
./autogen.sh \
    --enable-mingw-cross-compile \
    --with-distro=LibreOfficeWin64 \
    --host=x86_64-w64-mingw32

# Build
make
```

### 4. Alternative: Build Linux Version in WSL
```bash
# Configure for Linux
./autogen.sh \
    --enable-debug \
    --enable-dbgutil \
    --without-java \
    --without-help

# Build
make
```

## Option 3: Use LODE (LibreOffice Development Environment)

LODE automates the entire setup process.

1. Download LODE from https://wiki.documentfoundation.org/Development/lode
2. Run the setup script - it will:
   - Install all required dependencies
   - Configure the build environment
   - Set up the correct paths
3. Follow the LODE instructions to build

## Build Configuration Options

### Common Options
- `--enable-debug` - Include debug symbols
- `--enable-dbgutil` - Enable debug utilities
- `--without-java` - Skip Java support (faster build)
- `--without-help` - Skip help files (faster build)
- `--disable-odk` - Skip Office Development Kit
- `--with-parallelism=N` - Use N parallel build jobs

### Windows-Specific Options
- `--with-visual-studio=2019` or `--with-visual-studio=2022`
- `--with-windows-sdk=10.0.19041.0` - Specify Windows SDK version
- `--enable-64-bit` - Build 64-bit version
- `--with-distro=LibreOfficeWin64` - Use official Win64 configuration

## Build Output

After a successful build:
- **Executables**: `instdir/program/`
- **Libraries**: `instdir/program/`
- **Installer packages**: `workdir/installation/`

## Troubleshooting

### Clock Skew Error
```bash
# Fix time sync in WSL
sudo hwclock -s

# Or clean and rebuild
make clean
find . -type f -exec touch {} +
make
```

### Missing Dependencies
Check that all Cygwin packages are installed correctly. You can re-run Cygwin setup to add missing packages.

### Visual Studio Path Issues
Ensure the INCLUDE and LIB environment variables point to your actual VS installation paths.

### Build Failures
- Check `config.log` for configuration errors
- Run `make verbose=true` for detailed build output
- Consider using `--disable-dependency-tracking` for troubleshooting

## Additional Resources

- Official Wiki: https://wiki.documentfoundation.org/Development/BuildingOnWindows
- Build Instructions: https://wiki.documentfoundation.org/Development/How_to_build
- Developer Guide: https://wiki.documentfoundation.org/Development