# Launch 3 Claude instances in parallel and monitor their actual work

Write-Host "Launching 3 Claude instances simultaneously..." -ForegroundColor Green

# Define tasks for each instance
$tasks = @(
    @{
        Name = "Instance1-Filters"
        Prompt = "You are creating Google Docs filter definitions. Create these files NOW:
1. filter/source/config/fragments/filters/Google_Docs_Document.xcu with filter for Google Docs
2. filter/source/config/fragments/filters/Google_Sheets_Spreadsheet.xcu for Google Sheets  
3. filter/source/config/fragments/filters/Google_Slides_Presentation.xcu for Google Slides
4. filter/source/config/fragments/types/Google_Docs_Document.xcu
5. filter/source/config/fragments/types/Google_Sheets_Spreadsheet.xcu
6. filter/source/config/fragments/types/Google_Slides_Presentation.xcu

Use service names like com.sun.star.comp.gdocs.DocsImportFilter. Create actual working filter definitions based on existing LibreOffice filters."
        LogFile = ".claude\orchestrator\logs\inst1_filters.log"
    },
    @{
        Name = "Instance2-Converters"
        Prompt = "You are implementing Google Docs converters. Create these files NOW in ucb/source/ucp/gdocs/:
1. gdocs_docconverter.hxx and .cxx - Convert Google Docs to ODF Writer format
2. gdocs_sheetconverter.hxx and .cxx - Convert Google Sheets to ODF Calc format
3. gdocs_slideconverter.hxx and .cxx - Convert Google Slides to ODF Impress format

Implement actual conversion logic using Google Drive API export to OOXML then convert to ODF."
        LogFile = ".claude\orchestrator\logs\inst2_converters.log"
    },
    @{
        Name = "Instance3-UI"
        Prompt = "You are implementing UI integration. Do these tasks NOW:
1. Modify fpicker/source/office/RemoteFilesDialog.cxx to add Google Drive as a service
2. Create fpicker/source/office/GoogleDriveService.hxx and .cxx for file picker
3. Modify sfx2/source/dialog/filedlghelper.cxx to handle gdocs:// URLs
4. Create test files in ucb/qa/cppunit/gdocs/

Actually modify/create the code files with working implementations."
        LogFile = ".claude\orchestrator\logs\inst3_ui.log"
    }
)

# Start all instances
$jobs = @()
foreach ($task in $tasks) {
    Write-Host "Starting $($task.Name)..." -ForegroundColor Yellow
    
    $job = Start-Process -FilePath "claude.cmd" `
        -ArgumentList "--print", "`"$($task.Prompt)`"" `
        -NoNewWindow `
        -PassThru `
        -RedirectStandardOutput $task.LogFile `
        -RedirectStandardError "$($task.LogFile).err"
    
    $jobs += @{
        Process = $job
        Name = $task.Name
        LogFile = $task.LogFile
    }
}

Write-Host "`nAll 3 instances running in parallel!" -ForegroundColor Green
Write-Host "Monitoring their work..." -ForegroundColor Cyan

# Monitor loop
$startTime = Get-Date
while ($true) {
    Clear-Host
    Write-Host "=== PARALLEL CLAUDE INSTANCES MONITOR ===" -ForegroundColor Cyan
    Write-Host "Runtime: $([int]((Get-Date) - $startTime).TotalSeconds) seconds`n" -ForegroundColor Yellow
    
    foreach ($job in $jobs) {
        $status = if ($job.Process.HasExited) { "COMPLETED" } else { "RUNNING" }
        $color = if ($job.Process.HasExited) { "Green" } else { "Yellow" }
        
        Write-Host "$($job.Name): " -NoNewline
        Write-Host $status -ForegroundColor $color
        
        if (Test-Path $job.LogFile) {
            Write-Host "Latest output:" -ForegroundColor Gray
            Get-Content $job.LogFile -Tail 3 | ForEach-Object {
                Write-Host "  $_" -ForegroundColor DarkGray
            }
        }
        Write-Host ""
    }
    
    # Check if all completed
    $running = $jobs | Where-Object { -not $_.Process.HasExited }
    if ($running.Count -eq 0) {
        Write-Host "All instances completed!" -ForegroundColor Green
        break
    }
    
    Write-Host "Press Ctrl+C to stop monitoring..." -ForegroundColor DarkGray
    Start-Sleep -Seconds 2
}

Write-Host "`nChecking created files..." -ForegroundColor Yellow