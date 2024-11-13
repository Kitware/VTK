$erroractionpreference = "stop"

$version = "0.29.0"
$sha256sum = "55AB902C0F1F0FBBBFFE083DCA4691A4874E1FDE64CEAACC13E1399BF79F5825"
$filename = "buildcache-windows"
$tarball = "$filename.zip"

$outdir = $pwd.Path
$outdir = "$outdir\.gitlab"
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://gitlab.com/bits-n-bites/buildcache/-/releases/v$version/downloads/$tarball" -OutFile "$outdir\$tarball"
$hash = Get-FileHash "$outdir\$tarball" -Algorithm SHA256
if ($hash.Hash -ne $sha256sum) {
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$outdir\$tarball", "$outdir")
