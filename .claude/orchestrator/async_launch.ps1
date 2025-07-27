# Launch Claude instances asynchronously with real-time monitoring

Write-Host "Starting 3 Claude instances asynchronously..." -ForegroundColor Green

# Create monitoring files
$null = New-Item -ItemType Directory -Force -Path ".claude\orchestrator\status"

# Launch Instance 1 asynchronously
$process1 = Start-Process -FilePath "claude.cmd" `
    -ArgumentList "--print", "You are Instance 1 implementing file picker integration. Modify fpicker/source/office/RemoteFilesDialog.cxx to add Google Drive. Create fpicker/source/office/GoogleDriveService.hxx and .cxx. Write status updates frequently. Create actual code files NOW." `
    -NoNewWindow -PassThru `
    -RedirectStandardOutput ".claude\orchestrator\logs\live_instance1.log" `
    -RedirectStandardError ".claude\orchestrator\logs\live_instance1_err.log"

# Launch Instance 2 asynchronously  
$process2 = Start-Process -FilePath "claude.cmd" `
    -ArgumentList "--print", "You are Instance 2 implementing URL handling. Modify sfx2/source/dialog/filedlghelper.cxx for gdocs:// URLs. Add Google Drive to Places sidebar. Write status updates frequently. Create actual code modifications NOW." `
    -NoNewWindow -PassThru `
    -RedirectStandardOutput ".claude\orchestrator\logs\live_instance2.log" `
    -RedirectStandardError ".claude\orchestrator\logs\live_instance2_err.log"

# Launch Instance 3 asynchronously
$process3 = Start-Process -FilePath "claude.cmd" `
    -ArgumentList "--print", "You are Instance 3 creating test suite. Create ucb/qa/cppunit/gdocs/test_gdocs_auth.cxx, test_gdocs_provider.cxx, test_gdocs_converters.cxx. Write status updates frequently. Create actual test files NOW." `
    -NoNewWindow -PassThru `
    -RedirectStandardOutput ".claude\orchestrator\logs\live_instance3.log" `
    -RedirectStandardError ".claude\orchestrator\logs\live_instance3_err.log"

Write-Host "All 3 instances launched with PIDs: $($process1.Id), $($process2.Id), $($process3.Id)" -ForegroundColor Yellow

# Real-time monitoring function
function Show-Progress {
    param($ProcessList)
    
    Clear-Host
    Write-Host "=== LIVE MONITORING - UI INTEGRATION ===" -ForegroundColor Cyan
    Write-Host "Time: $(Get-Date -Format 'HH:mm:ss')" -ForegroundColor Yellow
    Write-Host ""
    
    foreach ($i in 1..3) {
        $process = $ProcessList[$i-1]
        $logFile = ".claude\orchestrator\logs\live_instance$i.log"
        $errFile = ".claude\orchestrator\logs\live_instance$i`_err.log"
        
        $status = if ($process.HasExited) { "COMPLETED" } else { "RUNNING" }
        $color = if ($process.HasExited) { "Green" } else { "Yellow" }
        
        Write-Host "Instance $i - " -NoNewline
        Write-Host $status -ForegroundColor $color -NoNewline
        Write-Host " (PID: $($process.Id))"
        
        # Show latest output
        if (Test-Path $logFile) {
            $content = Get-Content $logFile -Tail 3 -ErrorAction SilentlyContinue
            if ($content) {
                Write-Host "  Latest:" -ForegroundColor Gray
                $content | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }
            } else {
                Write-Host "  [No output yet...]" -ForegroundColor DarkRed
            }
        }
        
        # Show errors if any
        if (Test-Path $errFile) {
            $errors = Get-Content $errFile -Tail 1 -ErrorAction SilentlyContinue
            if ($errors) {
                Write-Host "  ERROR: $errors" -ForegroundColor Red
            }
        }
        Write-Host ""
    }
}

# Monitor loop
$processes = @($process1, $process2, $process3)
$startTime = Get-Date

do {
    Show-Progress -ProcessList $processes
    
    # Check if any are stuck (no output for too long)
    foreach ($i in 1..3) {
        $logFile = ".claude\orchestrator\logs\live_instance$i.log"
        if (Test-Path $logFile) {
            $lastWrite = (Get-Item $logFile).LastWriteTime
            $timeSinceUpdate = (Get-Date) - $lastWrite
            if ($timeSinceUpdate.TotalMinutes -gt 2 -and -not $processes[$i-1].HasExited) {
                Write-Host "WARNING: Instance $i hasn't updated in $([int]$timeSinceUpdate.TotalMinutes) minutes!" -ForegroundColor Red
            }
        }
    }
    
    $running = $processes | Where-Object { -not $_.HasExited }
    
    if ($running.Count -eq 0) {
        Write-Host "All instances completed!" -ForegroundColor Green
        break
    }
    
    Write-Host "Press Ctrl+C to stop monitoring..." -ForegroundColor DarkGray
    Start-Sleep -Seconds 5
    
} while ($true)

Write-Host "`nFinal status check..." -ForegroundColor Yellow