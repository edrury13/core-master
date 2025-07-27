@echo off
echo Starting LibreOffice from Docker build...
echo.
echo Starting X11 apps first to test connection...
docker exec -d libreoffice-dev bash -c "DISPLAY=172.28.160.1:0.0 xclock"

echo.
echo Starting LibreOffice Writer...
docker exec libreoffice-dev bash -c "cd /build/instdir/program && DISPLAY=172.28.160.1:0.0 timeout 5 ./soffice --writer --nofirststartwizard || echo 'Note: Process forking error is expected with Docker'"

echo.
echo If you see "ERROR 4 forking process", check your VcXsrv window.
echo The application may still be starting despite the error.
echo.
echo Make sure VcXsrv is running with:
echo - Multiple windows mode
echo - "Disable access control" checked
echo.
pause