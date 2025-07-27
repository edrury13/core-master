# LibreOffice VNC Development Environment

This setup provides a complete LibreOffice development environment with VNC GUI access, allowing you to build and test LibreOffice from source code in a containerized environment with full desktop GUI support.

## Quick Start

### 1. Build the Docker Image

**Windows:**
```powershell
.\build.ps1
```

**Linux/macOS:**
```bash
chmod +x *.sh
./build-docker.sh
```

### 2. Start the VNC Environment

**Windows:**
```powershell
.\run-vnc.ps1
```

**Linux/macOS:**
```bash
./run-docker.sh
```

**Using Docker Compose:**
```bash
docker-compose up -d libreoffice-vnc
```

### 3. Access the GUI

- **Web Browser (recommended):** http://localhost:6080/vnc.html
- **VNC Client:** localhost:5901
- **Password:** libreoffice

## What You Get

- **Complete LibreOffice Build Environment** - All dependencies pre-installed
- **XFCE Desktop Environment** - Lightweight desktop with full GUI support
- **VNC Server** - Remote desktop access via VNC protocol
- **noVNC Web Interface** - Browser-based VNC client (no software installation required)
- **Desktop Shortcuts** - One-click building and running of LibreOffice applications
- **Persistent Storage** - Build artifacts and cache preserved between container restarts

## Desktop Shortcuts

Once connected via VNC, you'll find these shortcuts on the desktop:

1. **Build LibreOffice** - Configures and builds LibreOffice from source
2. **LibreOffice Writer** - Launches Writer from your build
3. **LibreOffice Calc** - Launches Calc from your build

## Building LibreOffice

### Option 1: Desktop Shortcut
1. Connect via VNC (web or client)
2. Double-click "Build LibreOffice" on the desktop
3. Wait 30-60 minutes for build to complete

### Option 2: Terminal
```bash
# Connect to container
docker exec -it libreoffice-vnc bash

# Run build script
/home/builder/build-libreoffice.sh
```

### Option 3: From VNC Desktop
1. Open terminal in VNC session
2. Run: `/home/builder/build-libreoffice.sh`

## Running LibreOffice Applications

After building, you can run LibreOffice applications:

### From Desktop Shortcuts
- Double-click "LibreOffice Writer" or "LibreOffice Calc"

### From Terminal
```bash
# In VNC terminal or container
/home/builder/run-libreoffice.sh writer
/home/builder/run-libreoffice.sh calc
/home/builder/run-libreoffice.sh draw
/home/builder/run-libreoffice.sh impress
```

## Directory Structure

- `/core` - Your LibreOffice source code (read-only mount)
- `/build` - Build directory where compilation happens
- `/ccache` - Compiler cache for faster rebuilds
- `/home/builder` - User home directory with persistent settings

## Build Configuration

The build is configured with development-friendly options:
- Debug symbols enabled
- Debug utilities enabled
- Development symbols included
- Java disabled (faster build)
- Help files disabled (faster build)
- MySpell dictionaries disabled (faster build)

## Port Configuration

- **5901** - VNC server port
- **6080** - noVNC web interface port

## Persistent Volumes

The environment uses Docker volumes for persistence:
- `libreoffice-build` - Build artifacts and compiled binaries
- `libreoffice-ccache` - Compiler cache for faster rebuilds
- `libreoffice-home` - User home directory settings

## Environment Variables

You can customize the environment:

```bash
# Change VNC resolution
-e VNC_RESOLUTION=1920x1080

# Change build parallelism
-e PARALLELISM=8

# Change VNC password
-e VNC_PASSWORD=mypassword
```

## Docker Compose Services

### Main VNC Service
```bash
docker-compose up -d libreoffice-vnc
```

### Build-Only Service (no GUI)
```bash
docker-compose run --rm libreoffice-build
```

### X11 Forwarding (Linux only)
```bash
docker-compose --profile x11 up -d libreoffice-x11
```

## Troubleshooting

### Can't Access VNC
1. Check container is running: `docker ps`
2. Check ports are exposed: `docker port libreoffice-vnc`
3. Check logs: `docker logs libreoffice-vnc`

### Build Fails
1. Check available disk space: `df -h`
2. Check logs: `docker exec libreoffice-vnc tail -f /home/builder/build.log`
3. Clean build: Delete volume and rebuild

### Slow Performance
1. Increase Docker resources (CPU/Memory)
2. Increase build parallelism: `-e PARALLELISM=8`
3. Use SSD for Docker storage

## Development Workflow

1. **Edit Code** - Modify source files on your host system
2. **Build** - Use VNC desktop shortcut or terminal to build
3. **Test** - Run LibreOffice applications via VNC GUI
4. **Debug** - Full GUI debugging available in VNC session

## Security Notes

- Container runs with non-root user `builder`
- VNC password is configurable
- Source code mounted read-only
- No external network access during build

## Clean Up

### Stop Container
```bash
docker stop libreoffice-vnc
```

### Remove Container
```bash
docker rm libreoffice-vnc
```

### Remove Volumes (WARNING: Deletes all build artifacts)
```bash
docker volume rm libreoffice-build libreoffice-ccache libreoffice-home
```

## Advanced Usage

### Custom Build Options
Edit `build-libreoffice.sh` and modify the `autogen.sh` parameters for different build configurations.

### Multiple Parallel Builds
Run multiple containers with different names for parallel development.

### Integration with IDEs
Use VS Code with Remote-Containers extension for integrated development.

## System Requirements

- **CPU:** 4+ cores recommended
- **RAM:** 8GB minimum, 16GB recommended for comfortable building
- **Storage:** 20GB+ free space for build artifacts
- **Docker:** Recent version with BuildKit support

This environment provides a complete, isolated LibreOffice development setup that works consistently across Windows, macOS, and Linux hosts.