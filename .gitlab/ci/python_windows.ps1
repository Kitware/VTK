# Sets up a Python for Windows CI jobs and sets `PYTHON_PREFIX`. This script
# must be dot-sourced. Python is installed via `uv` from python-build-standalone,
# which provides relocatable (and free-threaded) interpreters. See
# `.gitlab/ci/python_macos.sh` for the rationale.
#
# The version request comes from `$env:PYTHON_VERSION_SUFFIX` (e.g. "3.14t"),
# else from `CMAKE_CONFIGURATION` (e.g. `wheel_windows312_x86_64`), else
# defaults to the general CI version.

powershell -File ".gitlab/ci/uv.ps1"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$env:UV_CACHE_DIR = "$pwdpath\.gitlab\uv-cache"
$env:UV_PYTHON_INSTALL_DIR = "$pwdpath\.gitlab\uv-python"

$python_request = "3.14"
if ($env:PYTHON_VERSION_SUFFIX) {
    $python_request = $env:PYTHON_VERSION_SUFFIX
} elseif ("$env:CMAKE_CONFIGURATION" -match "windows3(\d+)") {
    $python_request = "3.$($Matches[1])"
}

& "$pwdpath\.gitlab\uv.exe" python install --no-bin "cpython-$python_request"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$python_exe = & "$pwdpath\.gitlab\uv.exe" python find --managed-python "cpython-$python_request"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Set-Item -Force -Path "env:PYTHON_PREFIX" -Value (Split-Path -Parent $python_exe)
