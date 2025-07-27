docker run -it `
  --name libreoffice-vnc `
  -p 5901:5901 `
  -p 6080:6080 `
  -v core-master_libreoffice-build:/build `
  --entrypoint /bin/bash `
  libreoffice-builder-vnc:latest