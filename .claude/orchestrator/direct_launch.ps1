# Direct async launch of Claude instances with monitoring

Write-Host "Launching Claude instances directly..." -ForegroundColor Green

# Set up environment
$claudePath = "C:\Users\drury\AppData\Roaming\npm\claude.cmd"
$workDir = Get-Location

# Create three separate script files for each instance
$script1 = @"
`$env:PATH = "C:\Users\drury\AppData\Roaming\npm;`$env:PATH"
Set-Location "$workDir"
& claude --print "You are Instance 1. IMMEDIATELY start modifying fpicker/source/office/RemoteFilesDialog.cxx to add Google Drive service. Create GoogleDriveService.hxx and .cxx files. Work on actual code files NOW. Report what files you are creating."
"@

$script2 = @"
`$env:PATH = "C:\Users\drury\AppData\Roaming\npm;`$env:PATH"
Set-Location "$workDir"
& claude --print "You are Instance 2. IMMEDIATELY modify sfx2/source/dialog/filedlghelper.cxx to handle gdocs:// URLs. Add Google Drive to file dialog places. Work on actual code modifications NOW. Report what you are changing."
"@

$script3 = @"
`$env:PATH = "C:\Users\drury\AppData\Roaming\npm;`$env:PATH"
Set-Location "$workDir"
& claude --print "You are Instance 3. IMMEDIATELY create test files in ucb/qa/cppunit/gdocs/: test_gdocs_auth.cxx, test_gdocs_provider.cxx, test_gdocs_converters.cxx. Work on actual test code NOW. Report what tests you are writing."
"@

# Save scripts to temp files
$script1 | Out-File -FilePath ".claude\orchestrator\temp_script1.ps1" -Encoding UTF8
$script2 | Out-File -FilePath ".claude\orchestrator\temp_script2.ps1" -Encoding UTF8  
$script3 | Out-File -FilePath ".claude\orchestrator\temp_script3.ps1" -Encoding UTF8

# Launch all three asynchronously
Write-Host "Starting Instance 1..." -ForegroundColor Yellow
$job1 = Start-Job -ScriptBlock {
    param($scriptPath)
    & powershell -ExecutionPolicy Bypass -File $scriptPath
} -ArgumentList (Resolve-Path ".claude\orchestrator\temp_script1.ps1").Path

Write-Host "Starting Instance 2..." -ForegroundColor Yellow
$job2 = Start-Job -ScriptBlock {
    param($scriptPath)
    & powershell -ExecutionPolicy Bypass -File $scriptPath
} -ArgumentList (Resolve-Path ".claude\orchestrator\temp_script2.ps1").Path

Write-Host "Starting Instance 3..." -ForegroundColor Yellow
$job3 = Start-Job -ScriptBlock {
    param($scriptPath)
    & powershell -ExecutionPolicy Bypass -File $scriptPath
} -ArgumentList (Resolve-Path ".claude\orchestrator\temp_script3.ps1").Path

Write-Host "`nAll 3 instances running! Job IDs: $($job1.Id), $($job2.Id), $($job3.Id)" -ForegroundColor Green

# Real-time monitoring
$startTime = Get-Date
do {
    Clear-Host
    Write-Host "=== ASYNC UI INTEGRATION MONITOR ===" -ForegroundColor Cyan
    Write-Host "Runtime: $([int]((Get-Date) - $startTime).TotalSeconds) seconds" -ForegroundColor Yellow
    Write-Host ""
    
    $jobs = @(
        @{Job=$job1; Name="Instance 1 - File Picker"; Color="Green"},
        @{Job=$job2; Name="Instance 2 - Dialog Helper"; Color="Yellow"},
        @{Job=$job3; Name="Instance 3 - Test Suite"; Color="Cyan"}
    )
    
    foreach ($jobInfo in $jobs) {
        $job = $jobInfo.Job
        $status = $job.State
        $color = if ($status -eq 'Running') { "Yellow" } else { $jobInfo.Color }
        
        Write-Host "$($jobInfo.Name): " -NoNewline
        Write-Host $status.ToUpper() -ForegroundColor $color
        
        # Try to get any output
        if ($job.HasMoreData) {
            $output = Receive-Job -Job $job -Keep | Select-Object -Last 2
            if ($output) {
                $output | ForEach-Object { 
                    Write-Host "  > $_" -ForegroundColor DarkGray 
                }
            }
        } else {
            Write-Host "  [Working...]" -ForegroundColor DarkGray
        }
        Write-Host ""
    }
    
    $running = @($job1, $job2, $job3) | Where-Object { $_.State -eq 'Running' }
    
    if ($running.Count -eq 0) {
        Write-Host "✓ All instances completed!" -ForegroundColor Green
        break
    }
    
    Start-Sleep -Seconds 3
    
} while ($true)

# Collect final results
Write-Host "`nCollecting results..." -ForegroundColor Yellow
$result1 = Receive-Job -Job $job1
$result2 = Receive-Job -Job $job2
$result3 = Receive-Job -Job $job3

$result1 | Out-File ".claude\orchestrator\logs\final_instance1.log"
$result2 | Out-File ".claude\orchestrator\logs\final_instance2.log"
$result3 | Out-File ".claude\orchestrator\logs\final_instance3.log"

Remove-Job $job1, $job2, $job3
Remove-Item ".claude\orchestrator\temp_script*.ps1"

Write-Host "✓ UI Integration completed! Check final logs." -ForegroundColor Green