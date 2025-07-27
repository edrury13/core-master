#!/bin/bash

echo "Starting Ubuntu VNC container with LibreOffice build..."

docker run -d \
  --name ubuntu-vnc-libre \
  -p 5901:5901 \
  -p 6080:6080 \
  -v core-master_libreoffice-build:/build \
  -e VNC_PW=password \
  consol/ubuntu-xfce-vnc:latest

echo "Container started!"
echo "Access VNC at:"
echo "  - Web browser: http://localhost:6080/?password=password"
echo "  - VNC client: localhost:5901 (password: password)"
echo ""
echo "To run LibreOffice in the container:"
echo "  1. Open terminal in VNC desktop"
echo "  2. Run: /build/instdir/program/soffice"