#!/bin/bash
# Run LibreOffice from Docker build

docker exec libreoffice-dev bash -c "
export DISPLAY=172.28.160.1:0.0
export SAL_USE_VCLPLUGIN=gen
export LIBGL_ALWAYS_SOFTWARE=1
export LD_LIBRARY_PATH=/build/instdir/program:\$LD_LIBRARY_PATH
cd /build/instdir/program
echo 'Starting LibreOffice Writer...'
exec /build/instdir/program/oosplash --writer --nofirststartwizard
"