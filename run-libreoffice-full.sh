#!/bin/bash
# Run LibreOffice with full desktop environment

docker run -it --rm \
  -v core-master_libreoffice-build:/build \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  ubuntu:22.04 \
  bash -c "
    echo 'Installing dependencies...' && \
    apt-get update && \
    apt-get install -y \
      xfce4 \
      xfce4-terminal \
      libreoffice \
      --no-install-recommends && \
    apt-get remove -y libreoffice* && \
    echo 'Starting LibreOffice...' && \
    /build/instdir/program/soffice
  "