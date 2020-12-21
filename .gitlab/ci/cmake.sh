#!/bin/sh

set -e

readonly version="3.19.2"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        sha256sum="4d8a6d852c530f263b22479aad196416bb4406447e918bd9759c6593b7f5f3f9"
        platform="Linux"
        arch="x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="50afa2cb66bea6a0314ef28034f3ff1647325e30cf5940f97906a56fd9640bd8"
        platform="macos"
        arch="universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform
readonly arch

readonly filename="cmake-$version-$platform-$arch"
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
