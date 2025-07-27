docker run -d `
  --name ubuntu22-vnc `
  -p 5901:5901 `
  -p 6080:6080 `
  -v core-master_libreoffice-build:/build `
  -e DEBIAN_FRONTEND=noninteractive `
  ubuntu:22.04 `
  bash -c "
    apt-get update && 
    apt-get install -y xfce4 xfce4-terminal tightvncserver novnc websockify sudo &&
    apt-get install -y libx11-6 libxrender1 libxext6 libxtst6 libxi6 libxinerama1 libxrandr2 libxcursor1 libcairo2 libcups2 libdbus-1-3 libfontconfig1 libfreetype6 libgcc-s1 libgl1 libglu1-mesa libsm6 libice6 libxcomposite1 libxdamage1 libxfixes3 libgtk-3-0 libglib2.0-0 libharfbuzz0b libpango-1.0-0 libnss3 libnspr4 &&
    mkdir -p ~/.vnc &&
    echo 'password' | vncpasswd -f > ~/.vnc/passwd &&
    chmod 600 ~/.vnc/passwd &&
    echo '#!/bin/bash' > ~/.vnc/xstartup &&
    echo 'startxfce4 &' >> ~/.vnc/xstartup &&
    chmod +x ~/.vnc/xstartup &&
    vncserver :1 -geometry 1920x1080 -depth 24 &&
    websockify -D --web=/usr/share/novnc/ 6080 localhost:5901 &&
    echo 'VNC Started! Access at http://localhost:6080' &&
    echo 'To run LibreOffice: /build/instdir/program/soffice' &&
    tail -f /dev/null
  "