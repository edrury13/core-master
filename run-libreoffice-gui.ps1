# Run LibreOffice with GUI in Ubuntu 22.04
docker run -it --rm `
  -v core-master_libreoffice-build:/build `
  -e DISPLAY=host.docker.internal:0.0 `
  ubuntu:22.04 `
  bash -c "apt update && apt install -y libx11-6 libxrender1 libxext6 libxtst6 libxi6 libxinerama1 libxrandr2 libxcursor1 libcairo2 libcups2 libdbus-1-3 libfontconfig1 libfreetype6 libgcc-s1 libgl1 libglu1-mesa libsm6 libice6 libxcomposite1 libxdamage1 libxfixes3 libgtk-3-0 libglib2.0-0 libharfbuzz0b libpango-1.0-0 libnss3 libnspr4 && /build/instdir/program/soffice"