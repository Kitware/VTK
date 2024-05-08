$erroractionpreference = "stop"

$prefix = "emsdk"
$version = "3.1.58"
$sha256sum = "B612123122A2747682F6E80091E973956E38C34CEA35C867CAA1BB740EC910B4"
$filename = "$prefix-$version"
$tarball = "$filename.zip"

$outdir = $pwd.Path
$outdir = "$outdir\.gitlab"
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://github.com/emscripten-core/emsdk/archive/refs/tags/$version.zip" -OutFile "$outdir\$tarball"
$hash = Get-FileHash "$outdir\$tarball" -Algorithm SHA256
if ($hash.Hash -ne $sha256sum) {
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$outdir\$tarball", "$outdir")
Move-Item -Path "$outdir\$filename" -Destination "$outdir\$prefix"
