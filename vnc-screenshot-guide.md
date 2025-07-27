# VNC Connection Visual Guide

## What the Connection Looks Like:

### TightVNC Viewer Connection Window:
```
┌─────────────────────────────────────┐
│ TightVNC Viewer                     │
├─────────────────────────────────────┤
│ Remote Host: [172.17.0.2::5900]     │
│                                     │
│ [Connect]  [Options]  [Listen]      │
└─────────────────────────────────────┘
```

### After Connecting, You'll See:
- A window with the XFCE desktop
- A panel at the bottom with applications menu
- Desktop icons

### To Run LibreOffice:
1. Right-click on the desktop
2. Select "Open Terminal Here"
3. In the terminal, type:
   ```
   cd /build/instdir/program
   ./soffice --writer
   ```

## Alternative: If TightVNC doesn't work, try Chrome VNC Extension:

1. Open Chrome browser
2. Install "VNC Viewer for Google Chrome" extension
3. Open the extension and enter: 172.17.0.2 port 5900
4. Connect without password