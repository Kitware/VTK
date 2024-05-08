# Get all instances of chrome process
$chromeProcesses = Get-Process -Name "chrome" -ErrorAction SilentlyContinue

# Check if any chrome processes are found
if ($chromeProcesses) {
    # Iterate through each process and kill it
    foreach ($process in $chromeProcesses) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        Write-Host "Killed process $($process.Name) with ID $($process.Id)"
    }
} else {
    Write-Host "No chrome processes found."
}
