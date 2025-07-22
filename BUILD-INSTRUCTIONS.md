# LibreOffice Build Instructions

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [Platform-Specific Instructions](#platform-specific-instructions)
4. [Build Configuration](#build-configuration)
5. [Building](#building)
6. [Running](#running)
7. [Development Builds](#development-builds)
8. [Troubleshooting](#troubleshooting)

## Prerequisites

### Hardware Requirements
- **RAM**: Minimum 4GB, recommended 8GB or more
- **Disk Space**: ~50GB free space for source + build
- **CPU**: Multi-core recommended for faster builds

### Software Requirements
- Git
- Compiler toolchain (platform-specific)
- Build dependencies (platform-specific)

## Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/LibreOffice/core.git libreoffice
cd libreoffice
```

### 2. Run autogen.sh
```bash
./autogen.sh
```

### 3. Build
```bash
make
```

### 4. Run
```bash
instdir/program/soffice
```

## Platform-Specific Instructions

### Linux (Ubuntu/Debian)

#### Install Dependencies
```bash
# Update package list
sudo apt-get update

# Install basic build tools
sudo apt-get install -y \
    build-essential git autoconf automake libtool pkg-config \
    python3-dev python3-pip flex bison gperf nasm

# Install LibreOffice dependencies
sudo apt-get build-dep libreoffice

# Or manually install key dependencies
sudo apt-get install -y \
    libx11-dev libxext-dev libxrender-dev libxrandr-dev \
    libgl1-mesa-dev libglu1-mesa-dev libgtk-3-dev \
    libcups2-dev libfontconfig1-dev libxinerama-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libcairo2-dev libkrb5-dev libnss3-dev \
    libxml2-dev libxslt1-dev libpython3-dev \
    openjdk-11-jdk ant junit4 \
    libboost-dev libhunspell-dev libhyphen-dev \
    libmythes-dev liblpsolve55-dev libcppunit-dev \
    libclucene-dev libexpat1-dev libmysqlclient-dev \
    libpq-dev firebird-dev libcurl4-openssl-dev
```

#### Configure and Build
```bash
# Basic configuration
./autogen.sh \
    --enable-debug \
    --without-java \
    --without-help \
    --without-myspell-dicts

# Full build with all features
./autogen.sh

# Build with parallel jobs
make -j$(nproc)
```

### Windows

#### Using Cygwin

1. **Install Cygwin** (64-bit) with these packages:
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

2. **Install Visual Studio 2019/2022**
   - Include "Desktop development with C++"
   - Include Windows 10 SDK

3. **Set up environment**
```bash
# In Cygwin terminal
export INCLUDE="C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/include"
export LIB="C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.29.30133/lib/x64"
```

4. **Configure and build**
```bash
./autogen.sh \
    --with-visual-studio=2019 \
    --with-windows-sdk=10.0.19041.0 \
    --enable-debug \
    --without-java

make
```

#### Using WSL2 (Recommended)

1. **Install WSL2** with Ubuntu
2. Follow Linux instructions above
3. Use X server (VcXsrv or similar) for GUI

### macOS

#### Install Dependencies

1. **Install Xcode** and Command Line Tools
```bash
xcode-select --install
```

2. **Install Homebrew**
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

3. **Install build dependencies**
```bash
brew install autoconf automake libtool pkg-config nasm
brew install python@3.9 openjdk@11 ant
brew install boost hunspell hyphen mythes lpsolve
brew install icu4c openssl@1.1 libpng libjpeg-turbo
```

4. **Configure and build**
```bash
# Set up environment
export PATH="/usr/local/opt/python@3.9/bin:$PATH"
export JAVA_HOME=$(/usr/libexec/java_home -v 11)

# Configure
./autogen.sh \
    --enable-debug \
    --with-macosx-sdk=11.0 \
    --with-macosx-version-min-required=10.15

# Build
make
```

## Build Configuration

### Common Configuration Options

```bash
# Minimal debug build (fastest)
./autogen.sh \
    --enable-debug \           # Debug symbols
    --enable-dbgutil \        # Debug utilities
    --without-java \          # Skip Java
    --without-help \          # Skip help files
    --without-myspell-dicts \ # Skip dictionaries
    --disable-odk \           # Skip SDK
    --disable-online-update   # Skip update checks

# Release build
./autogen.sh \
    --enable-release-build \
    --with-vendor="My Build" \
    --with-lang="en-US de fr es"

# Development build with tests
./autogen.sh \
    --enable-debug \
    --enable-dbgutil \
    --enable-werror \         # Warnings as errors
    --with-junit \            # Enable Java tests
    --with-cppunit            # Enable C++ tests
```

### Using distro-configs

```bash
# Use predefined configuration
./autogen.sh --with-distro=LibreOfficeLinux

# Available distro configs in distro-configs/
# - LibreOfficeLinux.conf
# - LibreOfficeWin32.conf
# - LibreOfficeMacOSX.conf
# - LibreOfficeAndroid.conf
```

## Building

### Full Build
```bash
# Single-threaded (slow but reliable)
make

# Parallel build (faster)
make -j$(nproc)              # Linux
make -j$(sysctl -n hw.ncpu)  # macOS
make -j%NUMBER_OF_PROCESSORS% # Windows

# Verbose build
make VERBOSE=1
```

### Incremental Build
```bash
# Build specific module
make sw                    # Build Writer
make sc                    # Build Calc
make sd                    # Build Impress/Draw

# Rebuild specific module
make sw.clean && make sw

# Build and run tests
make check                 # All tests
make CppunitTest_sw_uiwriter  # Specific test
```

### Build Output
- **instdir/**: Installation directory with runnable LibreOffice
- **workdir/**: Intermediate build files
- **solver/**: (deprecated) Old build output location

## Running

### Running from Build Directory

```bash
# Run LibreOffice
instdir/program/soffice

# Run specific application
instdir/program/soffice --writer  # Writer
instdir/program/soffice --calc    # Calc
instdir/program/soffice --impress # Impress
instdir/program/soffice --draw    # Draw
instdir/program/soffice --base    # Base
instdir/program/soffice --math    # Math

# Run with clean profile
instdir/program/soffice --safe-mode

# Run with custom user profile
instdir/program/soffice -env:UserInstallation=file:///tmp/libreoffice-profile
```

### Installing

```bash
# Create installation packages
make distro-pack-install

# Install to system (Linux)
sudo make install DESTDIR=/opt/libreoffice

# Create DMG (macOS)
make macosx-package-DMG

# Create MSI (Windows)
make windows-package-msi
```

## Development Builds

### Debug Options

```bash
# Run with debugging
SAL_USE_VCLPLUGIN=gen instdir/program/soffice  # Generic VCL plugin
SAL_FORCEGL=1 instdir/program/soffice          # Force OpenGL
SAL_FORCESKIA=1 instdir/program/soffice        # Force Skia

# Debug output
SAL_LOG="+INFO.vcl+WARN" instdir/program/soffice

# GDB debugging
gdb instdir/program/soffice.bin
(gdb) run --writer

# LLDB debugging (macOS)
lldb instdir/LibreOffice.app/Contents/MacOS/soffice
(lldb) run --writer
```

### Using ccache

```bash
# Install ccache
sudo apt-get install ccache  # Linux
brew install ccache          # macOS

# Configure with ccache
./autogen.sh --enable-ccache

# Check ccache stats
ccache -s
```

### Using icecream

```bash
# Install icecream
sudo apt-get install icecc

# Configure
./autogen.sh --enable-icecream

# Set up scheduler
icecc-scheduler -d
```

## Troubleshooting

### Common Build Errors

#### Missing Dependencies
```bash
# Re-run autogen to check dependencies
./autogen.sh

# Install missing packages based on error messages
```

#### Out of Memory
```bash
# Reduce parallel jobs
make -j2

# Increase swap (Linux)
sudo dd if=/dev/zero of=/swapfile bs=1G count=8
sudo mkswap /swapfile
sudo swapon /swapfile
```

#### Build Failures
```bash
# Clean and rebuild
make clean
make

# Clean specific module
make sw.clean
make sw

# Complete clean
make distclean
./autogen.sh
make
```

### Development Tips

#### Faster Builds
1. Use `--disable-symbols` for faster linking
2. Use `--without-java` if not needed
3. Use `--without-help` to skip help compilation
4. Enable ccache or icecream
5. Build only needed modules

#### IDE Setup
```bash
# Generate IDE project files

# VS Code
make vscode-ide-integration

# Vim (YouCompleteMe)
make vim-ide-integration

# QtCreator
make qtcreator-ide-integration

# Xcode (macOS)
make xcode-ide-integration
```

### Getting Help

- **Mailing List**: libreoffice@lists.freedesktop.org
- **IRC**: #libreoffice-dev on irc.libera.chat
- **Wiki**: https://wiki.documentfoundation.org/Development
- **Gerrit**: https://gerrit.libreoffice.org/

## Advanced Topics

### Cross-Compilation

```bash
# Android
./autogen.sh \
    --with-distro=LibreOfficeAndroid \
    --with-android-ndk=/path/to/android-ndk \
    --with-android-sdk=/path/to/android-sdk

# iOS
./autogen.sh \
    --with-distro=LibreOfficeiOS \
    --host=arm64-apple-darwin
```

### Custom Builds

```bash
# Minimal build for testing
./autogen.sh \
    --disable-gui \
    --disable-scripting \
    --disable-database-connectivity \
    --disable-extensions \
    --disable-report-builder

# Build with custom branding
./autogen.sh \
    --with-branding=/path/to/branding \
    --with-vendor="My Company" \
    --with-about-background=/path/to/image.png
```

---

For more detailed information, see:
- [Development Wiki](https://wiki.documentfoundation.org/Development)
- [Building on Linux](https://wiki.documentfoundation.org/Development/BuildingOnLinux)
- [Building on Windows](https://wiki.documentfoundation.org/Development/BuildingOnWindows)
- [Building on macOS](https://wiki.documentfoundation.org/Development/BuildingOnMac)