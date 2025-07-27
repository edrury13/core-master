#!/bin/bash
# Run LibreOffice with all required dependencies

docker run -it --rm \
  -v core-master_libreoffice-build:/build \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  ubuntu:22.04 \
  bash -c "
    apt-get update && \
    apt-get install -y \
      libx11-6 \
      libxrender1 \
      libxext6 \
      libxtst6 \
      libxi6 \
      libxinerama1 \
      libxrandr2 \
      libxcursor1 \
      libcairo2 \
      libcups2 \
      libdbus-1-3 \
      libfontconfig1 \
      libfreetype6 \
      libgcc-s1 \
      libgl1 \
      libglu1-mesa \
      libsm6 \
      libice6 \
      libxcomposite1 \
      libxdamage1 \
      libxfixes3 \
      libatk1.0-0 \
      libgtk-3-0 \
      libglib2.0-0 \
      libharfbuzz0b \
      libpango-1.0-0 \
      libpangocairo-1.0-0 \
      libpangoft2-1.0-0 \
      libnss3 \
      libnspr4 && \
    /build/instdir/program/soffice
  "