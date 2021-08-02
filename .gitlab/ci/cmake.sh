#!/bin/sh

set -e

readonly version="3.21.1"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        sha256sum="bf496ce869d0aa8c1f57e4d1a2e50c8f2fb12a6cd7ccb37ad743bb88f6b76a1e"
        platform="linux-x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="9dc2978c4d94a44f71336fa88c15bb0eee47cf44b6ece51b10d1dfae95f82279"
        platform="macos-universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
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
