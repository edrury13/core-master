# Simple async launch with monitoring

Write-Host "Launching 3 Claude instances..." -ForegroundColor Green

# Launch jobs
$job1 = Start-Job -ScriptBlock {
    & claude --print "Instance 1: Modify fpicker/source/office/RemoteFilesDialog.cxx to add Google Drive. Create GoogleDriveService.hxx/cxx files. Report progress as you work on actual code files."
}

$job2 = Start-Job -ScriptBlock {
    & claude --print "Instance 2: Modify sfx2/source/dialog/filedlghelper.cxx to handle gdocs:// URLs. Add Google Drive to file dialog places. Report progress as you modify actual code."
}

$job3 = Start-Job -ScriptBlock {
    & claude --print "Instance 3: Create test suite files in ucb/qa/cppunit/gdocs/: test_gdocs_auth.cxx, test_gdocs_provider.cxx, test_gdocs_converters.cxx. Report progress as you create actual test files."
}

Write-Host "Jobs launched: $($job1.Id), $($job2.Id), $($job3.Id)" -ForegroundColor Yellow

# Monitor
$start = Get-Date
while ($true) {
    $running = @($job1, $job2, $job3) | Where-Object { $_.State -eq 'Running' }
    $completed = @($job1, $job2, $job3) | Where-Object { $_.State -eq 'Completed' }
    
    Write-Host "`r[$([int]((Get-Date) - $start).TotalSeconds)s] Running: $($running.Count) | Completed: $($completed.Count)" -NoNewline
    
    if ($running.Count -eq 0) {
        break
    }
    
    Start-Sleep 2
}

Write-Host "`nAll completed! Getting results..." -ForegroundColor Green

# Save results
Receive-Job $job1 | Out-File ".claude\orchestrator\logs\ui_job1_final.log"
Receive-Job $job2 | Out-File ".claude\orchestrator\logs\ui_job2_final.log"
Receive-Job $job3 | Out-File ".claude\orchestrator\logs\ui_job3_final.log"

Remove-Job $job1, $job2, $job3
Write-Host "Done!" -ForegroundColor Green