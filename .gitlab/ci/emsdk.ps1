$erroractionpreference = "stop"

$prefix = "emsdk"
$version = "3.1.64"
$sha256sum = "2A61FC9D271CAB441918B9495D1103C56B5062FD46C721E2D988A6DBBDF6CD01"
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
