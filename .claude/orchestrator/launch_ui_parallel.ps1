# Launch multiple Claude instances simultaneously for UI integration

Write-Host "Launching UI integration instances in parallel..." -ForegroundColor Green

# Create status tracking directory
New-Item -ItemType Directory -Force -Path ".claude\orchestrator\status" | Out-Null

# Instance 1: File Picker Integration
$job1 = Start-Job -ScriptBlock {
    $env:PATH = "C:\Users\drury\AppData\Roaming\npm;$env:PATH"
    $prompt = @"
You are Instance 1 implementing Google Drive file picker integration.

TASKS:
1. Modify fpicker/source/office/RemoteFilesDialog.cxx to add Google Drive service
2. Create fpicker/source/office/GoogleDriveService.hxx and .cxx
3. Add Google Drive authentication UI

REQUIREMENTS:
- Add Google Drive to services list in RemoteFilesDialog
- Implement OAuth2 authentication dialog
- Handle folder browsing and file selection
- Create actual working code now

Write progress to .claude\orchestrator\status\ui_instance1.txt
"@
    & claude --print $prompt
} -Name "UI-Instance1"

# Instance 2: File Dialog Helper
$job2 = Start-Job -ScriptBlock {
    $env:PATH = "C:\Users\drury\AppData\Roaming\npm;$env:PATH"
    $prompt = @"
You are Instance 2 implementing gdocs:// URL handling.

TASKS:
1. Modify sfx2/source/dialog/filedlghelper.cxx to handle gdocs:// URLs
2. Add Google Drive to Places sidebar in file dialogs
3. Implement URL scheme recognition

REQUIREMENTS:
- Parse gdocs:// URLs properly
- Integrate with existing UCP system
- Add Google Drive bookmark to file dialog places
- Create actual working code now

Write progress to .claude\orchestrator\status\ui_instance2.txt
"@
    & claude --print $prompt
} -Name "UI-Instance2"

# Instance 3: Test Suite
$job3 = Start-Job -ScriptBlock {
    $env:PATH = "C:\Users\drury\AppData\Roaming\npm;$env:PATH"
    $prompt = @"
You are Instance 3 creating the comprehensive test suite.

TASKS:
1. Create ucb/qa/cppunit/gdocs/test_gdocs_auth.cxx - OAuth2 authentication tests
2. Create ucb/qa/cppunit/gdocs/test_gdocs_provider.cxx - Content provider tests
3. Create ucb/qa/cppunit/gdocs/test_gdocs_converters.cxx - Document conversion tests
4. Create ucb/qa/cppunit/gdocs/Makefile - Build configuration

REQUIREMENTS:
- Use CppUnit framework
- Test authentication flow
- Test file operations (list, get, put)
- Test document conversions
- Create actual working test code now

Write progress to .claude\orchestrator\status\ui_instance3.txt
"@
    & claude --print $prompt
} -Name "UI-Instance3"

Write-Host "All 3 instances launched simultaneously!" -ForegroundColor Yellow
Write-Host "Job IDs: $($job1.Id), $($job2.Id), $($job3.Id)" -ForegroundColor Cyan

# Real-time monitoring loop
$startTime = Get-Date
Write-Host "`nMonitoring parallel execution..." -ForegroundColor Green

while ($true) {
    Clear-Host
    Write-Host "=== PARALLEL UI INTEGRATION - LIVE MONITOR ===" -ForegroundColor Cyan
    Write-Host "Runtime: $([int]((Get-Date) - $startTime).TotalSeconds) seconds" -ForegroundColor Yellow
    Write-Host ""
    
    # Check each job status
    $jobs = @($job1, $job2, $job3)
    $running = $jobs | Where-Object { $_.State -eq 'Running' }
    $completed = $jobs | Where-Object { $_.State -eq 'Completed' }
    
    Write-Host "Status: Running=$($running.Count) | Completed=$($completed.Count)" -ForegroundColor White
    Write-Host ""
    
    # Show individual status
    @(
        @{Job=$job1; Name="Instance 1 - File Picker"; Color="Green"},
        @{Job=$job2; Name="Instance 2 - Dialog Helper"; Color="Yellow"}, 
        @{Job=$job3; Name="Instance 3 - Test Suite"; Color="Cyan"}
    ) | ForEach-Object {
        $status = if ($_.Job.State -eq 'Running') { "RUNNING" } else { $_.Job.State }
        Write-Host "$($_.Name): " -NoNewline
        Write-Host $status -ForegroundColor $_.Color
        
        # Show any available output
        if ($_.Job.HasMoreData) {
            $output = Receive-Job -Job $_.Job -Keep | Select-Object -Last 2
            $output | ForEach-Object { Write-Host "  > $_" -ForegroundColor DarkGray }
        }
    }
    
    if ($running.Count -eq 0) {
        Write-Host "`n✓ ALL INSTANCES COMPLETED!" -ForegroundColor Green
        break
    }
    
    Start-Sleep -Seconds 3
}

# Collect outputs
Write-Host "`nCollecting results..." -ForegroundColor Yellow
$output1 = Receive-Job -Job $job1
$output2 = Receive-Job -Job $job2
$output3 = Receive-Job -Job $job3

# Save outputs to files
$output1 | Out-File ".claude\orchestrator\logs\ui_instance1_output.log"
$output2 | Out-File ".claude\orchestrator\logs\ui_instance2_output.log"
$output3 | Out-File ".claude\orchestrator\logs\ui_instance3_output.log"

# Cleanup
Remove-Job -Job $job1, $job2, $job3

Write-Host "✓ UI Integration complete! Check logs for results." -ForegroundColor Green