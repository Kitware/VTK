#!/bin/sh

set -e

readonly prefix="emsdk"
readonly version="3.1.64"

shatool="sha256sum"
sha256sum="2a61fc9d271cab441918b9495d1103c56b5062fd46c721e2d988a6dbbdf6cd01"
readonly shatool
readonly sha256sum

readonly filename="$prefix-$version"
readonly tarball="$filename.zip"

cd .gitlab
echo "$sha256sum  $tarball" > emsdk.sha256sum
curl -L "https://github.com/emscripten-core/emsdk/archive/refs/tags/$version.zip" --output "$tarball"
$shatool --check emsdk.sha256sum
./cmake/bin/cmake -E tar xf "$tarball"
mv $filename $prefix
