# Setting up X Server for LibreOffice GUI on Windows

To see and interact with LibreOffice's GUI when running in Docker on Windows, you need an X server. Here are your options:

## Option 1: VcXsrv (Recommended - Free)

1. **Download VcXsrv**
   - Download from: https://sourceforge.net/projects/vcxsrv/
   - Install with default options

2. **Configure VcXsrv**
   - Run XLaunch from Start Menu
   - Select **"Multiple windows"** (or "One large window" if preferred)
   - Display number: **0**
   - Select **"Start no client"**
   - **Important**: Check **"Disable access control"**
   - Optional: Save configuration for future use
   - Click Finish to start the X server

3. **Firewall Configuration**
   - Windows Firewall may prompt you
   - Allow VcXsrv through both Private and Public networks
   - X server uses port 6000

## Option 2: X410 (Paid - from Microsoft Store)

1. **Install X410**
   - Purchase and install from Microsoft Store
   - Run X410

2. **Configure X410**
   - Right-click X410 icon in system tray
   - Select "Allow Public Access"
   - Choose windowing mode (Windowed Desktop recommended)

## Option 3: WSLg (Windows 11 / Windows 10 with WSL2)

If you have Windows 11 or Windows 10 with WSL2 and WSLg installed:
- No additional X server needed
- GUI support is built-in
- Just run the scripts

## Running LibreOffice with GUI

Once your X server is running:

```powershell
# Run the full LibreOffice suite
.\run-libreoffice-gui.ps1

# Run specific applications
.\run-libreoffice-gui.ps1 -Writer
.\run-libreoffice-gui.ps1 -Calc
.\run-libreoffice-gui.ps1 -Impress
.\run-libreoffice-gui.ps1 -Draw
```

## Troubleshooting

1. **"Cannot open display" error**
   - Make sure X server is running
   - Check "Disable access control" in VcXsrv
   - Verify Windows Firewall isn't blocking port 6000

2. **Slow performance**
   - This is normal for X11 forwarding
   - Try different rendering options in X server settings
   - Use "Native OpenGL" if available

3. **Black or corrupted window**
   - Add these environment variables in the container:
     ```
     export LIBGL_ALWAYS_SOFTWARE=1
     export SAL_USE_VCLPLUGIN=gen
     ```

4. **Connection refused**
   - Check if X server is actually running
   - Verify the IP address detection in the script
   - Try manually setting DISPLAY to your host IP:
     ```powershell
     $env:DISPLAY = "192.168.1.100:0.0"  # Replace with your IP
     ```

## Performance Tips

- VcXsrv with "Multiple windows" mode typically performs better than "One large window"
- Disable window animations in LibreOffice for smoother experience
- Consider using RDP or VNC alternatives for better performance over network