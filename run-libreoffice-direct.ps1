# Run LibreOffice directly with X11 forwarding
docker run -it --rm `
  -v core-master_libreoffice-build:/build `
  -e DISPLAY=host.docker.internal:0 `
  ubuntu:22.04 `
  bash -c "apt-get update && apt-get install -y libx11-6 libxrender1 libxext6 libxtst6 libxi6 && /build/instdir/program/soffice"