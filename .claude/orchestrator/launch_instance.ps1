param(
    [Parameter(Mandatory=$true)]
    [int]$InstanceNumber,
    
    [Parameter(Mandatory=$true)]
    [string]$TaskFile
)

$prompt = @"
You are Instance $InstanceNumber working on the LibreOffice project. 

IMPORTANT: You must follow these guidelines:
1. Read your task assignment from: $TaskFile
2. Create status updates in: .claude/orchestrator/status/instance${InstanceNumber}_status.md
3. Work ONLY on the files assigned to you in the task file
4. Test all changes before marking tasks complete
5. Update your status file every 30 minutes with progress

Start by reading your task file and beginning implementation. Focus on producing high-quality, well-tested code that follows LibreOffice coding standards.
"@

# Launch Claude with the task
& claude --print $prompt