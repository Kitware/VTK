$erroractionpreference = "stop"

$version = "0.12.1"
$sha256sum = "8fcb0cb46e1229065e344758980924e569bef5882ef45f46fada8fb24e06b74a"
$filename = "uv-x86_64-pc-windows-msvc"
$tarball = "$filename.zip"

$outdir = $pwd.Path
$outdir = "$outdir\.gitlab"
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://github.com/astral-sh/uv/releases/download/$version/$tarball" -OutFile "$outdir\$tarball"
$hash = Get-FileHash "$outdir\$tarball" -Algorithm SHA256
if ($hash.Hash -ne $sha256sum) {
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$outdir\$tarball", "$outdir\uv-bin")
Move-Item -Force "$outdir\uv-bin\uv.exe" "$outdir\uv.exe"
Remove-Item -Recurse -Force "$outdir\uv-bin", "$outdir\$tarball"
