# Quick Start Guide - LibreOffice VNC Development

## Current Status
Your LibreOffice VNC container is currently setting up. The package installation takes 5-10 minutes on first run.

## Check Container Status
```powershell
docker ps
docker logs libreoffice-quick
```

## Once VNC is Ready (wait 5-10 minutes)
1. **Web Browser Access:** http://localhost:6080/vnc.html
2. **VNC Client:** localhost:5901
3. **Password:** libreoffice

## Immediate Alternative - Direct Container Access

While VNC is setting up, you can work directly in the container:

```powershell
# Enter the container
docker exec -it libreoffice-quick bash

# Copy source code to build directory
cp -r /core/* /build/
cd /build

# Configure LibreOffice build
./autogen.sh --enable-debug --without-java --without-help --without-myspell-dicts --with-parallelism=4

# Build LibreOffice (takes 30-60 minutes)
make -j4

# The built LibreOffice will be at: /build/instdir/program/soffice
```

## Test VNC Connection
```powershell
# Check if VNC server is running
docker exec libreoffice-quick ps aux | grep vnc

# Check if noVNC web service is running
docker exec libreoffice-quick netstat -tlnp | grep 6080
```

## Build Status Check
```powershell
# Check if build is complete
docker exec libreoffice-quick ls -la /build/instdir/program/soffice
```

## Stop/Restart Container
```powershell
# Stop
docker stop libreoffice-quick

# Start again
docker start libreoffice-quick

# Remove (if you want to start fresh)
docker rm libreoffice-quick
```

## Next Steps

1. **Wait for VNC Setup** (5-10 minutes) - Check with `docker logs libreoffice-quick`
2. **Access via Browser** - Go to http://localhost:6080/vnc.html
3. **Start Building** - Use the terminal in VNC or direct container access
4. **Test LibreOffice** - Run built applications in VNC environment

The VNC setup is automated and will be ready shortly!