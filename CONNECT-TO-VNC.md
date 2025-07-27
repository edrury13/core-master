# Connect to LibreOffice GUI via VNC

## VNC Server is Running!

The VNC server is now running inside the Docker container with XFCE desktop environment.

## Connection Details:
- **VNC Server**: `172.17.0.2:5900`
- **Password**: None (no password required)

## How to Connect:

### Option 1: TightVNC Viewer (Recommended)
1. Download from: https://www.tightvnc.com/download.php
2. Install and run TightVNC Viewer
3. Enter: `172.17.0.2::5900` in the connection field
4. Click Connect (no password needed)

### Option 2: RealVNC Viewer
1. Download from: https://www.realvnc.com/en/connect/download/viewer/
2. Install and run RealVNC Viewer
3. Enter: `172.17.0.2:5900`
4. Connect (no password)

### Option 3: Windows Remote Desktop (if you have VNC enabled)
Use any VNC client you prefer

## What You'll See:
- XFCE desktop environment
- You can launch LibreOffice from the terminal or application menu
- The LibreOffice build from your source code is at: `/build/instdir/program/soffice`

## To Run LibreOffice in VNC:
Once connected, open a terminal in the VNC session and run:
```bash
cd /build/instdir/program
./soffice --writer
```

## Troubleshooting:
- If connection fails, check Windows Firewall
- Make sure Docker is running
- Container must be running: `docker ps`

The VNC approach bypasses Docker's process forking limitations and gives you a full desktop environment for testing LibreOffice!