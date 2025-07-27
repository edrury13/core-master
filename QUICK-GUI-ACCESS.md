# LibreOffice GUI Access

The container is currently installing GUI components. This will take a few minutes.

## Once Ready, Access GUI Via:

### Option 1: Web Browser (EASIEST)
1. Open: http://localhost:6080/vnc.html
2. Click "Connect" 
3. No password needed!

### Option 2: VNC Client
- Connect to: localhost:5900
- No password

## Run LibreOffice:
1. Right-click on the desktop
2. Select "Terminal" or "Terminal Emulator"
3. Run these commands:
   ```
   cd /build/instdir/program
   ./soffice --writer
   ```

## Check if Ready:
Run this in PowerShell:
```powershell
docker exec libreoffice-dev bash -c "ps aux | grep -E 'Xvfb|x11vnc|websockify' | grep -v grep | wc -l"
```
If it shows 3 or more, the GUI is ready!

## If You Need to Restart:
```powershell
docker stop libreoffice-dev
docker rm libreoffice-dev
./start-libreoffice-gui.ps1
```