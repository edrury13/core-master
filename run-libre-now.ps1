docker run -it --rm `
  -v core-master_libreoffice-build:/lo `
  -e DISPLAY=host.docker.internal:0.0 `
  -v /tmp/.X11-unix:/tmp/.X11-unix `
  --cap-add=SYS_ADMIN `
  --security-opt apparmor=unconfined `
  ubuntu:22.04 `
  bash -c "apt update && apt install -y libx11-6 libxrender1 libxext6 libxtst6 libxi6 libxinerama1 libxrandr2 libxcursor1 libcairo2 libcups2 libdbus-1-3 libfontconfig1 libfreetype6 libgcc-s1 libgl1 libglu1-mesa libsm6 libice6 libxcomposite1 libxdamage1 libxfixes3 libgtk-3-0 libglib2.0-0 libharfbuzz0b libpango-1.0-0 libnss3 libnspr4 && export SAL_USE_VCLPLUGIN=gen && export SAL_DISABLE_OOSPLASH=1 && /lo/instdir/program/soffice"