# Launch Claude instances with actual prompts and monitor their work

Write-Host "Launching 3 Claude worker instances..." -ForegroundColor Green

# Instance 1: Google Docs Integration
$prompt1 = @"
You are Instance 1 implementing Google Docs integration for LibreOffice.

CRITICAL: You must actually implement the code, not just plan it. Create real files with working code.

Your tasks:
1. Create the directory: ucb/source/ucp/gdocs/
2. Copy and adapt files from ucb/source/ucp/cmis/ as templates
3. Implement these files with COMPLETE CODE:
   - gdocs_provider.hxx and .cxx
   - gdocs_content.hxx and .cxx
   - gdocs_datasupplier.hxx and .cxx
   - gdocs_auth.hxx and .cxx
   - ucpgdocs1.component

Key requirements:
- Use Google OAuth2 endpoints: https://accounts.google.com/o/oauth2/v2/auth
- Support Google Drive API v3: https://www.googleapis.com/drive/v3/
- Implement token refresh mechanism
- Follow LibreOffice UNO component patterns

Write status updates to: .claude/orchestrator/status/instance1_status.txt
When complete, create: .claude/orchestrator/status/instance1_complete.txt

Start implementing NOW. Create the directory first, then implement each file.
"@

# Instance 2: DOCX Improvements
$prompt2 = @"
You are Instance 2 improving DOCX handling in LibreOffice Writer.

CRITICAL: You must actually modify the code, not just analyze it. Make real improvements.

Your tasks:
1. Fix style handling in sw/source/writerfilter/dmapper/StyleSheetTable.cxx
2. Improve table borders in sw/source/writerfilter/dmapper/TablePropertiesHandler.cxx
3. Add content control support in sw/source/writerfilter/dmapper/SdtHelper.cxx

Specific fixes needed:
- Style inheritance for custom styles
- Theme color preservation (w:themeColor)
- Table border double/triple line styles
- Cell gradient shading
- Content controls: dropdowns, date pickers, checkboxes

Write status updates to: .claude/orchestrator/status/instance2_status.txt
When complete, create: .claude/orchestrator/status/instance2_complete.txt

Start by examining the current code, then implement the fixes.
"@

# Instance 3: PPTX Improvements
$prompt3 = @"
You are Instance 3 improving PPTX handling in LibreOffice Impress.

CRITICAL: You must actually implement improvements, not just plan them. Write real code.

Your tasks:
1. Add SmartArt support in oox/source/drawingml/diagram/
2. Enhance animations in oox/source/ppt/animationspersist.cxx
3. Fix shape geometry in oox/source/drawingml/customshapeproperties.cxx

Specific implementations:
- Convert SmartArt process/hierarchy layouts to shapes
- Add motion path animation support
- Fix gradient fills in custom shapes
- Improve text box positioning within shapes

Write status updates to: .claude/orchestrator/status/instance3_status.txt
When complete, create: .claude/orchestrator/status/instance3_complete.txt

Start implementing the SmartArt converter first.
"@

# Launch the instances
Start-Process claude -ArgumentList "--print", $prompt1 -WindowStyle Hidden -RedirectStandardOutput ".claude\orchestrator\logs\instance1_output.txt"
Start-Process claude -ArgumentList "--print", $prompt2 -WindowStyle Hidden -RedirectStandardOutput ".claude\orchestrator\logs\instance2_output.txt"
Start-Process claude -ArgumentList "--print", $prompt3 -WindowStyle Hidden -RedirectStandardOutput ".claude\orchestrator\logs\instance3_output.txt"

Write-Host "All instances launched. Starting monitoring..." -ForegroundColor Yellow