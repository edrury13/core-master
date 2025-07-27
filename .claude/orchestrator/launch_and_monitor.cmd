@echo off
echo Starting 3 Claude instances simultaneously...

REM Clear old logs
del /Q .claude\orchestrator\logs\phase2_*.log 2>nul

REM Instance 1: Google Docs Filters
start "Instance 1 - Filters" /B cmd /c claude --print "Create Google Docs filter definitions in filter/source/config/fragments/. Create filters/Google_Docs_Document.xcu, filters/Google_Sheets_Spreadsheet.xcu, filters/Google_Slides_Presentation.xcu. Also create corresponding type definitions in types/ folder. Use service names com.sun.star.comp.gdocs.*ImportFilter. Actually create the files now." > .claude\orchestrator\logs\phase2_inst1.log 2>&1

REM Instance 2: Converters
start "Instance 2 - Converters" /B cmd /c claude --print "Create Google Docs converters in ucb/source/ucp/gdocs/. Create gdocs_docconverter.hxx/cxx to convert Google Docs to ODF Writer. Create gdocs_sheetconverter.hxx/cxx for Sheets to Calc. Create gdocs_slideconverter.hxx/cxx for Slides to Impress. Use Google Drive export API approach. Actually implement the code now." > .claude\orchestrator\logs\phase2_inst2.log 2>&1

REM Instance 3: UI Integration
start "Instance 3 - UI" /B cmd /c claude --print "Implement Google Drive UI integration. Modify fpicker/source/office/RemoteFilesDialog.cxx to add Google Drive service. Create GoogleDriveService.hxx/cxx. Modify sfx2/source/dialog/filedlghelper.cxx for gdocs:// URLs. Create test suite in ucb/qa/cppunit/gdocs/. Actually create/modify these files now." > .claude\orchestrator\logs\phase2_inst3.log 2>&1

echo All instances launched!
echo.
echo Monitoring progress... Press Ctrl+C to stop
echo.

:monitor
cls
echo === CLAUDE INSTANCES MONITOR ===
echo.

echo Instance 1 - Filters:
powershell -Command "if (Test-Path '.claude\orchestrator\logs\phase2_inst1.log') { Get-Content '.claude\orchestrator\logs\phase2_inst1.log' -Tail 5 | ForEach-Object { Write-Host $_ -ForegroundColor Yellow } }"
echo.

echo Instance 2 - Converters:
powershell -Command "if (Test-Path '.claude\orchestrator\logs\phase2_inst2.log') { Get-Content '.claude\orchestrator\logs\phase2_inst2.log' -Tail 5 | ForEach-Object { Write-Host $_ -ForegroundColor Cyan } }"
echo.

echo Instance 3 - UI Integration:
powershell -Command "if (Test-Path '.claude\orchestrator\logs\phase2_inst3.log') { Get-Content '.claude\orchestrator\logs\phase2_inst3.log' -Tail 5 | ForEach-Object { Write-Host $_ -ForegroundColor Green } }"
echo.

timeout /t 3 /nobreak >nul
goto monitor