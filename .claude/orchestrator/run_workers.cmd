@echo off
echo Launching Claude worker instances...

REM Instance 1: Google Docs Integration
start "Claude Instance 1 - Google Docs" /MIN cmd /c "claude --print \"You are Instance 1 implementing Google Docs integration. Create the directory ucb/source/ucp/gdocs/ and implement: gdocs_provider.hxx/cxx, gdocs_content.hxx/cxx, gdocs_datasupplier.hxx/cxx, gdocs_auth.hxx/cxx with OAuth2 for Google Drive API v3. Base on CMIS template. Actually create the files with working code. Update .claude/orchestrator/status/instance1_status.txt with progress.\" > .claude\orchestrator\logs\instance1_run.log 2>&1"

REM Instance 2: DOCX Improvements  
start "Claude Instance 2 - DOCX" /MIN cmd /c "claude --print \"You are Instance 2 improving DOCX in Writer. Fix style handling in sw/source/writerfilter/dmapper/StyleSheetTable.cxx, improve table borders in TablePropertiesHandler.cxx, add content controls. Actually modify the code. Update .claude/orchestrator/status/instance2_status.txt with progress.\" > .claude\orchestrator\logs\instance2_run.log 2>&1"

REM Instance 3: PPTX Improvements
start "Claude Instance 3 - PPTX" /MIN cmd /c "claude --print \"You are Instance 3 improving PPTX in Impress. Add SmartArt support in oox/source/drawingml/diagram/, enhance animations in oox/source/ppt/animationspersist.cxx, fix shapes. Actually implement code. Update .claude/orchestrator/status/instance3_status.txt with progress.\" > .claude\orchestrator\logs\instance3_run.log 2>&1"

echo All instances launched. Check logs for output.