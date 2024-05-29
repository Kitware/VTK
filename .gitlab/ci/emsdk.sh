#!/bin/sh

set -e

readonly prefix="emsdk"
readonly version="3.1.60"

shatool="sha256sum"
sha256sum="a2c8ba8bf54bbd19d6b32f137f06566caa98356c319e60b4f11d97a079b2ecf2"
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
