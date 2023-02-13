#!/bin/sh

set -e

readonly mindeps_version="3.12.4"
readonly latest_version="3.21.1"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        mindeps_sha256sum="486edd6710b5250946b4b199406ccbf8f567ef0e23cfe38f7938b8c78a2ffa5f"
        mindeps_platform="Linux-x86_64"
        latest_sha256sum="bf496ce869d0aa8c1f57e4d1a2e50c8f2fb12a6cd7ccb37ad743bb88f6b76a1e"
        latest_platform="linux-x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        mindeps_sha256sum="95d76c00ccb9ecb5cb51de137de00965c5e8d34b2cf71556cf8ba40577d1cff3"
        mindeps_platform="Darwin-x86_64"
        latest_sha256sum="9dc2978c4d94a44f71336fa88c15bb0eee47cf44b6ece51b10d1dfae95f82279"
        latest_platform="macos-universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly mindeps_sha256sum
readonly mindeps_platform
readonly latest_sha256sum
readonly latest_platform

use_mindeps=false
if [ "$CI_JOB_STAGE" = "test" ]; then
    : # Skip old CMake for testing as some of the features are useful.
elif echo "$CMAKE_CONFIGURATION" | grep -q -e 'mindeps'; then
    use_mindeps=true
fi

readonly use_mindeps

if $use_mindeps; then
    version="$mindeps_version"
    sha256sum="$mindeps_sha256sum"
    platform="$mindeps_platform"
else
    version="$latest_version"
    sha256sum="$latest_sha256sum"
    platform="$latest_platform"
fi
readonly version
readonly sha256sum
readonly platform

readonly filename="cmake-$version-$platform"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
$shatool --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" cmake

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin cmake/bin
fi
