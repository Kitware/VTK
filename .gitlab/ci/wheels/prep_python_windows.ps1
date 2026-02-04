param (
    [string]$version,
    [string]$dirversion = "",
    [switch]$freethreading
)

$erroractionpreference = "stop"

if ($dirversion -Eq "") {
    $dirversion = $version
}

$maybe_free_threading = ""
$free_threading_flag = ""
if ($freethreading) {
    $maybe_free_threading = "t"
    $free_threading_flag = "Include_freethreaded=1"
}

$installer="python-$version-amd64.exe"
$outdir = $pwd.Path
$dirname = "python-$version$maybe_free_threading-windows-x86_64"
$installdir = "$outdir\$dirname"

$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://www.python.org/ftp/python/$dirversion/$installer" -OutFile "$outdir\$installer"

Start-Process -NoNewWindow -Wait "$outdir\$installer" -ArgumentList "/quiet /passive /repair $free_threading_flag InstallLauncherAllUsers=0 AssociateFiles=0 Shortcuts=0 PrependPath=0 InstallAllUsers=0 TargetDir=$installdir Include_launcher=0 Include_doc=0 Include_debug=0 Include_tcltk=0 Include_test=0"

Add-Type -AssemblyName System.IO.Compression.FileSystem
# FIXME: This misses the top-level directory and makes a tarbomb.
[System.IO.Compression.ZipFile]::CreateFromDirectory($installdir, "$outdir\$dirname.zip")
