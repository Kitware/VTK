#!/bin/sh

set -e

readonly version="3.13.5"

case "$(uname -s)-$(uname -m)" in
    Linux-x86_64)
        shatool="sha256sum"
        sha256sum="e2fd0080a6f0fc1ec84647acdcd8e0b4019770f48d83509e6a5b0b6ea27e5864"
        platform="Linux-x86_64"
        ;;
    Linux-aarch64)
        shatool="sha256sum"
        sha256sum="LINUX_AARCH64_SHA256SUM"
        platform="Linux-aarch64"
        ;;
    Darwin-*)
        shatool="shasum -a 256"
        sha256sum="e04bcd52c64c2ee44f6def2ac4d5a610f01b329d0db65aecc2bb5135602ebfe0"
        platform="Darwin-x86_64"
        ;;
    *)
        echo "Unrecognized platform $(uname -s)-$(uname -m)"
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
mv "$filename" "cmake$CMAKE_SUFFIX"
rm "$tarball" cmake.sha256sum

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin "cmake$CMAKE_SUFFIX/bin"
fi
