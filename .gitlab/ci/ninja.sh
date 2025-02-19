#!/bin/sh

set -e

readonly version="1.10.2"

case "$( uname -s )-$( uname -m )" in
    Linux-x86_64)
        shatool="sha256sum"
        sha256sum="763464859c7ef2ea3a0a10f4df40d2025d3bb9438fcb1228404640410c0ec22d"
        platform="linux"
        ;;
    Linux-aarch64)
        shatool="sha256sum"
        sha256sum="5c25c6570b0155e95fce5918cb95f1ad9870df5768653afe128db822301a05a1"
        platform="linux-aarch64"
        ;;
    Darwin-*)
        shatool="shasum -a 256"
        sha256sum="6fa359f491fac7e5185273c6421a000eea6a2f0febf0ac03ac900bd4d80ed2a5"
        platform="mac"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="ninja-$platform"
readonly tarball="$filename.zip"

cd .gitlab

echo "$sha256sum  $tarball" > ninja.sha256sum
curl -OL "https://github.com/ninja-build/ninja/releases/download/v$version/$tarball"
$shatool --check ninja.sha256sum
./cmake/bin/cmake -E tar xf "$tarball"
