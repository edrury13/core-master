  1. Monitor build progress:
  docker logs -f libreoffice-dev
  2. Once build completes, run LibreOffice with GUI:
  .\run-libreoffice-gui.ps1
  2. Or specific apps:
  .\run-libreoffice-gui.ps1 -Writer
  .\run-libreoffice-gui.ps1 -Calc
  3. For faster subsequent builds:
  .\build.ps1  # Incremental build