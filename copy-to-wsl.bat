@echo off
echo Copying Google Drive authentication changes to WSL...

set WSL_PATH=\\wsl$\Ubuntu\home\drury\libreoffice\core-master

echo Creating directories if needed...
if not exist "%WSL_PATH%\include\ucb" mkdir "%WSL_PATH%\include\ucb"

echo Copying new files...
copy /Y "include\ucb\gdocsauth.hxx" "%WSL_PATH%\include\ucb\"
copy /Y "ucb\source\ucp\gdocs\gdocs_authservice.cxx" "%WSL_PATH%\ucb\source\ucp\gdocs\"

echo Copying modified files...
copy /Y "cui\source\dialogs\GoogleDriveFilePicker.cxx" "%WSL_PATH%\cui\source\dialogs\"
copy /Y "ucb\source\ucp\gdocs\gdocs_auth.cxx" "%WSL_PATH%\ucb\source\ucp\gdocs\"
copy /Y "ucb\Library_ucpgdocs1.mk" "%WSL_PATH%\ucb\"

echo.
echo Done! Files copied to WSL.
echo.
echo Next steps:
echo 1. Open WSL terminal
echo 2. cd ~/libreoffice/core-master
echo 3. Run: make ucb.clean ^&^& make ucb
echo 4. Run: make cui.clean ^&^& make cui
echo 5. Run: make