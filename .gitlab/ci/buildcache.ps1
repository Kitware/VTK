$erroractionpreference = "stop"

$version = "0.27.6"
$sha256sum = "48631671fd26a6f5bb4b6755afb3cd9618194c74b93ec5abd633c22343045134"
$filename = "buildcache-windows"
$tarball = "$filename.zip"

$outdir = $pwd.Path
$outdir = "$outdir\.gitlab"
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://github.com/mbitsnbites/buildcache/releases/download/v$version/$tarball" -OutFile "$outdir\$tarball"
$hash = Get-FileHash "$outdir\$tarball" -Algorithm SHA256
if ($hash.Hash -ne $sha256sum) {
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$outdir\$tarball", "$outdir")
