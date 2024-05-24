#!/bin/sh

set -e

readonly prefix="emsdk"
readonly version="3.1.58"

shatool="sha256sum"
sha256sum="b612123122a2747682f6e80091e973956e38c34cea35c867caa1bb740ec910b4"
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
