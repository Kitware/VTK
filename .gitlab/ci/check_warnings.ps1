param(
  [Parameter(Mandatory=$true)]$BuildPath
)

$erroractionpreference = "stop"
$esc="$([char]27)"
$txt_yellow="$esc[33m"
$txt_clear="$esc[0m"

# Check for "Warning" in the last configuration log
$conf_file = (Get-ChildItem -Path  .\build\Testing\Temporary\LastConfigure* | Sort-Object -Property LastWriteTime | Select-Object -Last 1).FullName
$conf_warnings = Select-String -Path $conf_file -Pattern "Warning"
if ($conf_warnings -ne $null)
{
  Write-Host $txt_yellow"Configuration warnings detected, please check cdash-commit job"$txt_clear
  $host.SetShouldExit(47); exit 47
}

# Check that the number of build warnings is zero
$build_warnings = (type "$BuildPath\compile_num_warnings.log")
if ($build_warnings -ne "0")
{
  Write-Host $txt_yellow"Build warnings detected, please check cdash-commit job"$txt_clear
  $host.SetShouldExit(47); exit 47
}

exit 0
