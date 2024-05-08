$erroractionpreference = "stop"

$prefix = "node"
$version = "22.0.0"
$sha256sum = "32D639B47D4C0A651FF8F8D7D41A454168A3D4045BE37985F9A810CF8CEF6174"
$filename = "$prefix-v$version-win-x64"
$tarball = "$filename.zip"

$outdir = $pwd.Path
$outdir = "$outdir\.gitlab"
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://nodejs.org/download/release/v$version/$tarball" -OutFile "$outdir\$tarball"
$hash = Get-FileHash "$outdir\$tarball" -Algorithm SHA256
if ($hash.Hash -ne $sha256sum) {
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$outdir\$tarball", "$outdir")
Move-Item -Path "$outdir\$filename" -Destination "$outdir\$prefix"
