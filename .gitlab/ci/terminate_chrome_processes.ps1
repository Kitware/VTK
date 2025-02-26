# Get all instances of chrome process
$chromeProcesses = Get-Process -Name "chrome" -ErrorAction SilentlyContinue

# Check if any chrome processes are found
if ($chromeProcesses) {
    # Iterate through each process and kill it
    foreach ($process in $chromeProcesses) {
        # Only terminates chrome processes which started from CI.
        if ("$($process.Path)" -Like "*\.gitlab\chrome\chrome.exe") {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            Write-Host "Killed process $($process.Name) with ID $($process.Id)"
        }
    }
} else {
    Write-Host "No chrome processes found."
}
