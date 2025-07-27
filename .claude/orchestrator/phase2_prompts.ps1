# Launch 3 Claude instances simultaneously for next phase

$prompt1 = @'
You are Instance 1 working on Google Docs filter definitions.

Your tasks:
1. Create filter definitions in filter/source/config/fragments/filters/:
   - Google_Docs_Document.xcu
   - Google_Sheets_Spreadsheet.xcu  
   - Google_Slides_Presentation.xcu

2. Create type definitions in filter/source/config/fragments/types/:
   - Google_Docs_Document.xcu
   - Google_Sheets_Spreadsheet.xcu
   - Google_Slides_Presentation.xcu

3. Register filters in filter/source/config/cache/typedetection.xcu

Use these service names:
- com.sun.star.comp.gdocs.DocsImportFilter
- com.sun.star.comp.gdocs.SheetsImportFilter
- com.sun.star.comp.gdocs.SlidesImportFilter

Write status to .claude/orchestrator/status/phase2_instance1.txt
'@

$prompt2 = @'
You are Instance 2 implementing Google Docs converters.

Your tasks:
1. Create ucb/source/ucp/gdocs/gdocs_docconverter.hxx/cxx:
   - Convert Google Docs JSON to ODF Writer format
   - Handle text, styles, images, tables
   
2. Create ucb/source/ucp/gdocs/gdocs_sheetconverter.hxx/cxx:
   - Convert Google Sheets to ODF Calc format
   - Handle cells, formulas, charts
   
3. Create ucb/source/ucp/gdocs/gdocs_slideconverter.hxx/cxx:
   - Convert Google Slides to ODF Impress format
   - Handle slides, layouts, transitions

Use Google Drive export API to get OOXML then convert to ODF.

Write status to .claude/orchestrator/status/phase2_instance2.txt
'@

$prompt3 = @'
You are Instance 3 implementing UI integration.

Your tasks:
1. Modify fpicker/source/office/RemoteFilesDialog.cxx:
   - Add Google Drive as a service
   - Integrate with gdocs provider
   
2. Create fpicker/source/office/GoogleDriveService.hxx/cxx:
   - Implement file picker service for Google Drive
   - Handle authentication UI
   - Browse folders and files
   
3. Modify sfx2/source/dialog/filedlghelper.cxx:
   - Add Google Drive to Places sidebar
   - Handle gdocs:// URLs

4. Create test suite in ucb/qa/cppunit/gdocs/:
   - Test authentication flow
   - Test file operations
   - Test converters

Write status to .claude/orchestrator/status/phase2_instance3.txt
'@

Write-Host "Launching 3 Claude instances simultaneously..." -ForegroundColor Green

# Launch all three instances at the same time
$job1 = Start-Job -ScriptBlock { 
    param($prompt) 
    & claude --print $prompt 
} -ArgumentList $prompt1

$job2 = Start-Job -ScriptBlock { 
    param($prompt) 
    & claude --print $prompt 
} -ArgumentList $prompt2

$job3 = Start-Job -ScriptBlock { 
    param($prompt) 
    & claude --print $prompt 
} -ArgumentList $prompt3

Write-Host "All 3 instances launched and running in parallel!" -ForegroundColor Yellow
Write-Host "Job IDs: $($job1.Id), $($job2.Id), $($job3.Id)" -ForegroundColor Cyan

# Monitor the jobs
while ($true) {
    $running = @($job1, $job2, $job3) | Where-Object { $_.State -eq 'Running' }
    $completed = @($job1, $job2, $job3) | Where-Object { $_.State -eq 'Completed' }
    
    Write-Host "`rRunning: $($running.Count) | Completed: $($completed.Count)" -NoNewline
    
    if ($running.Count -eq 0) {
        break
    }
    
    Start-Sleep -Seconds 2
}

Write-Host "`nAll instances completed!" -ForegroundColor Green

# Get the output
$output1 = Receive-Job -Job $job1
$output2 = Receive-Job -Job $job2  
$output3 = Receive-Job -Job $job3

# Save outputs
$output1 | Out-File ".claude\orchestrator\logs\phase2_instance1.log"
$output2 | Out-File ".claude\orchestrator\logs\phase2_instance2.log"
$output3 | Out-File ".claude\orchestrator\logs\phase2_instance3.log"

# Clean up
Remove-Job -Job $job1, $job2, $job3