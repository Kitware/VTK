#!/bin/sh

set -e

readonly mindeps_version="3.12.4"
readonly latest_version="3.26.3"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        mindeps_sha256sum="486edd6710b5250946b4b199406ccbf8f567ef0e23cfe38f7938b8c78a2ffa5f"
        mindeps_platform="Linux-x86_64"
        latest_sha256sum="28d4d1d0db94b47d8dfd4f7dec969a3c747304f4a28ddd6fd340f553f2384dc2"
        latest_platform="linux-x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        mindeps_sha256sum="95d76c00ccb9ecb5cb51de137de00965c5e8d34b2cf71556cf8ba40577d1cff3"
        mindeps_platform="Darwin-x86_64"
        latest_sha256sum="2b44cc892dc68b42123b9517c5d903690785b7ef489af26abf2fe3f3a6f2a112"
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
