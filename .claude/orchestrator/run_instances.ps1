# Launch multiple Claude instances to work in parallel

# Instance 1: Google Docs API Integration
Start-Job -Name "Claude-Instance1" -ScriptBlock {
    $prompt = @"
You are Instance 1 working on LibreOffice Google Docs integration.

Your task:
1. Read the task file at .claude/orchestrator/instance1_task.md
2. Examine the existing CMIS implementation in ucb/source/ucp/cmis/
3. Create the new ucb/source/ucp/gdocs/ directory structure
4. Implement OAuth2 authentication for Google
5. Create basic Google Drive content provider
6. Write status updates to .claude/orchestrator/status/instance1_status.md

Start by creating the directory structure and basic files.
"@
    
    & claude --print $prompt
} | Out-File ".claude/orchestrator/logs/instance1_output.log"

# Instance 2: DOCX Improvements
Start-Job -Name "Claude-Instance2" -ScriptBlock {
    $prompt = @"
You are Instance 2 working on DOCX improvements for LibreOffice.

Your task:
1. Read the task file at .claude/orchestrator/instance2_task.md
2. Analyze sw/source/writerfilter/dmapper/ for current limitations
3. Document issues with style handling, tables, and content controls
4. Implement improvements to StyleSheetTable.cxx
5. Fix table border and shading issues
6. Write status updates to .claude/orchestrator/status/instance2_status.md

Start by analyzing the current implementation and documenting issues.
"@
    
    & claude --print $prompt
} | Out-File ".claude/orchestrator/logs/instance2_output.log"

# Instance 3: PPTX Improvements  
Start-Job -Name "Claude-Instance3" -ScriptBlock {
    $prompt = @"
You are Instance 3 working on PPTX improvements for LibreOffice.

Your task:
1. Read the task file at .claude/orchestrator/instance3_task.md
2. Analyze oox/source/ppt/ for SmartArt and animation limitations
3. Study the drawingml implementation
4. Plan SmartArt layout conversions
5. Improve animation persistence
6. Write status updates to .claude/orchestrator/status/instance3_status.md

Start by analyzing current SmartArt support and creating test cases.
"@
    
    & claude --print $prompt
} | Out-File ".claude/orchestrator/logs/instance3_output.log"

Write-Host "Launched 3 Claude instances. Monitoring their progress..."

# Monitor jobs
while ($true) {
    $jobs = Get-Job -Name "Claude-Instance*"
    $running = $jobs | Where-Object { $_.State -eq "Running" }
    $completed = $jobs | Where-Object { $_.State -eq "Completed" }
    
    Write-Host "Running: $($running.Count), Completed: $($completed.Count)"
    
    if ($running.Count -eq 0) {
        break
    }
    
    Start-Sleep -Seconds 30
}

Write-Host "All instances completed."