#!/bin/sh

set -e

readonly version="3.19.7"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        sha256sum="ba4a5f46aab500e0d8d952ee735dcfb0c870d326e851addc037c99eb1ea4b66c"
        platform="Linux"
        arch="x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="52036f8450fabb69bedc3c3eccc58202ae9eefe8358fa383978c7b142f931fa5"
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
