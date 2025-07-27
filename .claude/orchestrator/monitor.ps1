# Real-time monitoring of Claude instances

function Get-InstanceStatus {
    param($Instance)
    
    $status = @{
        Number = $Instance
        Status = "Not Started"
        LastUpdate = ""
        CurrentFile = ""
        Progress = 0
    }
    
    $statusFile = ".claude\orchestrator\status\instance$Instance`_status.txt"
    if (Test-Path $statusFile) {
        $content = Get-Content $statusFile -Raw
        if ($content -match "Status: (.+)") { $status.Status = $Matches[1] }
        if ($content -match "Working on: (.+)") { $status.CurrentFile = $Matches[1] }
        if ($content -match "Progress: (\d+)%") { $status.Progress = [int]$Matches[1] }
        $status.LastUpdate = (Get-Item $statusFile).LastWriteTime.ToString("HH:mm:ss")
    }
    
    return $status
}

# Main monitoring loop
while ($true) {
    Clear-Host
    
    Write-Host "╔════════════════════════════════════════════════════════════════════════╗" -ForegroundColor Cyan
    Write-Host "║          LibreOffice Integration - Live Instance Monitor               ║" -ForegroundColor Cyan
    Write-Host "╠════════════════════════════════════════════════════════════════════════╣" -ForegroundColor Cyan
    
    # Get status for all instances
    $instances = @(1..3) | ForEach-Object { Get-InstanceStatus -Instance $_ }
    
    # Display status table
    Write-Host "║ Inst │ Status              │ Progress │ Current Work                  ║" -ForegroundColor Cyan
    Write-Host "╠══════╪═══════════════════════╪══════════╪═══════════════════════════════╣" -ForegroundColor Cyan
    
    foreach ($inst in $instances) {
        $statusColor = switch ($inst.Status) {
            "Not Started" { "Gray" }
            "Working" { "Yellow" }
            "Testing" { "Cyan" }
            "Complete" { "Green" }
            default { "White" }
        }
        
        $progressBar = "[" + ("█" * [Math]::Floor($inst.Progress / 10)) + ("░" * (10 - [Math]::Floor($inst.Progress / 10))) + "]"
        
        Write-Host "║ " -NoNewline -ForegroundColor Cyan
        Write-Host ("{0,-4}" -f $inst.Number) -NoNewline -ForegroundColor White
        Write-Host " │ " -NoNewline -ForegroundColor Cyan
        Write-Host ("{0,-19}" -f $inst.Status) -NoNewline -ForegroundColor $statusColor
        Write-Host " │ " -NoNewline -ForegroundColor Cyan
        Write-Host ("{0,3}% " -f $inst.Progress) -NoNewline -ForegroundColor White
        Write-Host "$progressBar" -NoNewline -ForegroundColor Green
        Write-Host " │ " -NoNewline -ForegroundColor Cyan
        Write-Host ("{0,-29}" -f $inst.CurrentFile.Substring(0, [Math]::Min(29, $inst.CurrentFile.Length))) -NoNewline -ForegroundColor White
        Write-Host " ║" -ForegroundColor Cyan
    }
    
    Write-Host "╚════════════════════════════════════════════════════════════════════════╝" -ForegroundColor Cyan
    
    # Show recent activity
    Write-Host "`nRecent Activity:" -ForegroundColor Yellow
    
    Get-ChildItem ".claude\orchestrator\logs\*_live.log" | ForEach-Object {
        $instanceNum = $_.Name -replace '.*instance(\d+).*', '$1'
        $lastLine = Get-Content $_ -Tail 1 -ErrorAction SilentlyContinue
        if ($lastLine) {
            Write-Host "Instance $instanceNum`: $lastLine" -ForegroundColor DarkGray
        }
    }
    
    # Check completion
    $completeCount = (Get-ChildItem ".claude\orchestrator\status\*_complete.txt" -ErrorAction SilentlyContinue).Count
    if ($completeCount -eq 3) {
        Write-Host "`n✓ All instances completed successfully!" -ForegroundColor Green
        break
    }
    
    Write-Host "`nPress Ctrl+C to stop monitoring..." -ForegroundColor DarkGray
    Start-Sleep -Seconds 2
}