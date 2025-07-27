# Launch multiple Claude instances in parallel with monitoring

# Create monitoring dashboard
function Show-Dashboard {
    Clear-Host
    Write-Host "=== LibreOffice Integration Orchestrator ===" -ForegroundColor Cyan
    Write-Host "Time: $(Get-Date -Format 'HH:mm:ss')" -ForegroundColor Yellow
    Write-Host ""
    
    # Check each instance status
    @(1..3) | ForEach-Object {
        $statusFile = ".claude\orchestrator\status\instance$_`_status.txt"
        $logFile = ".claude\orchestrator\logs\instance$_`_live.log"
        
        Write-Host "Instance $_`: " -NoNewline -ForegroundColor Green
        
        if (Test-Path $statusFile) {
            $status = Get-Content $statusFile -Tail 1
            Write-Host $status -ForegroundColor White
        } else {
            Write-Host "Starting..." -ForegroundColor Gray
        }
        
        if (Test-Path $logFile) {
            $lastLines = Get-Content $logFile -Tail 3
            $lastLines | ForEach-Object { Write-Host "  $_" -ForegroundColor DarkGray }
        }
        Write-Host ""
    }
}

# Instance 1: Google Docs Integration
Start-Process -FilePath "cmd.exe" -ArgumentList "/c", "claude --print < .claude\orchestrator\prompts\instance1.txt > .claude\orchestrator\logs\instance1_live.log 2>&1" -WindowStyle Hidden

# Instance 2: DOCX Improvements  
Start-Process -FilePath "cmd.exe" -ArgumentList "/c", "claude --print < .claude\orchestrator\prompts\instance2.txt > .claude\orchestrator\logs\instance2_live.log 2>&1" -WindowStyle Hidden

# Instance 3: PPTX Improvements
Start-Process -FilePath "cmd.exe" -ArgumentList "/c", "claude --print < .claude\orchestrator\prompts\instance3.txt > .claude\orchestrator\logs\instance3_live.log 2>&1" -WindowStyle Hidden

Write-Host "Launched 3 Claude instances. Monitoring progress..." -ForegroundColor Green

# Monitor loop
while ($true) {
    Show-Dashboard
    
    # Check if all instances completed
    $completed = @()
    @(1..3) | ForEach-Object {
        if (Test-Path ".claude\orchestrator\status\instance$_`_complete.txt") {
            $completed += $_
        }
    }
    
    if ($completed.Count -eq 3) {
        Write-Host "`nAll instances completed!" -ForegroundColor Green
        break
    }
    
    Start-Sleep -Seconds 5
}