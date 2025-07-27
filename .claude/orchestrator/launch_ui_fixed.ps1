# Launch multiple Claude instances simultaneously for UI integration

Write-Host "Launching UI integration instances in parallel..." -ForegroundColor Green

# Instance 1: File Picker Integration
$job1 = Start-Job -ScriptBlock {
    $prompt = "You are Instance 1 implementing Google Drive file picker integration. Modify fpicker/source/office/RemoteFilesDialog.cxx to add Google Drive service. Create fpicker/source/office/GoogleDriveService.hxx and .cxx. Add Google Drive to services list and implement OAuth2 authentication dialog. Create actual working code now."
    & claude --print $prompt
}

# Instance 2: File Dialog Helper  
$job2 = Start-Job -ScriptBlock {
    $prompt = "You are Instance 2 implementing gdocs:// URL handling. Modify sfx2/source/dialog/filedlghelper.cxx to handle gdocs:// URLs. Add Google Drive to Places sidebar in file dialogs. Parse gdocs:// URLs and integrate with UCP system. Create actual working code now."
    & claude --print $prompt
}

# Instance 3: Test Suite
$job3 = Start-Job -ScriptBlock {
    $prompt = "You are Instance 3 creating test suite. Create ucb/qa/cppunit/gdocs/test_gdocs_auth.cxx for OAuth2 tests, test_gdocs_provider.cxx for content provider tests, test_gdocs_converters.cxx for conversion tests. Use CppUnit framework. Create actual working test code now."
    & claude --print $prompt
}

Write-Host "All 3 instances launched!" -ForegroundColor Yellow
Write-Host "Job IDs: $($job1.Id), $($job2.Id), $($job3.Id)" -ForegroundColor Cyan

# Monitor progress
$startTime = Get-Date
do {
    $running = @($job1, $job2, $job3) | Where-Object { $_.State -eq 'Running' }
    $completed = @($job1, $job2, $job3) | Where-Object { $_.State -eq 'Completed' }
    
    Write-Host "`rRunning: $($running.Count) | Completed: $($completed.Count) | Time: $([int]((Get-Date) - $startTime).TotalSeconds)s" -NoNewline
    Start-Sleep -Seconds 2
} while ($running.Count -gt 0)

Write-Host "`n`nAll instances completed!" -ForegroundColor Green

# Collect results
$output1 = Receive-Job -Job $job1
$output2 = Receive-Job -Job $job2  
$output3 = Receive-Job -Job $job3

# Save outputs
$output1 | Out-File ".claude\orchestrator\logs\ui_job1.log"
$output2 | Out-File ".claude\orchestrator\logs\ui_job2.log"
$output3 | Out-File ".claude\orchestrator\logs\ui_job3.log"

Remove-Job -Job $job1, $job2, $job3
Write-Host "Results saved to logs!" -ForegroundColor Green