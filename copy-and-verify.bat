@echo off
echo Copying files to WSL location...

REM Copy UI files
copy "C:\Users\drury\Documents\GitHub\core-master\cui\source\dialogs\GoogleDriveFilePicker.cxx" "C:\Users\drury\libreoffice\core-master\cui\source\dialogs\" /Y
copy "C:\Users\drury\Documents\GitHub\core-master\cui\source\dialogs\GoogleDriveFilePickerService.cxx" "C:\Users\drury\libreoffice\core-master\cui\source\dialogs\" /Y

REM Copy SFX2 files
copy "C:\Users\drury\Documents\GitHub\core-master\sfx2\source\appl\appopen.cxx" "C:\Users\drury\libreoffice\core-master\sfx2\source\appl\" /Y

REM Copy UCP files
copy "C:\Users\drury\Documents\GitHub\core-master\ucb\source\ucp\gdocs\gdocs_auth.cxx" "C:\Users\drury\libreoffice\core-master\ucb\source\ucp\gdocs\" /Y
copy "C:\Users\drury\Documents\GitHub\core-master\ucb\source\ucp\gdocs\gdocs_datasupplier.cxx" "C:\Users\drury\libreoffice\core-master\ucb\source\ucp\gdocs\" /Y
copy "C:\Users\drury\Documents\GitHub\core-master\ucb\source\ucp\gdocs\gdocs_content.cxx" "C:\Users\drury\libreoffice\core-master\ucb\source\ucp\gdocs\" /Y
copy "C:\Users\drury\Documents\GitHub\core-master\ucb\source\ucp\gdocs\gdocs_provider.cxx" "C:\Users\drury\libreoffice\core-master\ucb\source\ucp\gdocs\" /Y

echo Done!