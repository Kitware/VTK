#!/bin/sh

set -e

readonly version="1.12.1"

case "$( uname -s )-$( uname -m )" in
    Linux-x86_64)
        shatool="sha256sum"
        sha256sum="6f98805688d19672bd699fbbfa2c2cf0fc054ac3df1f0e6a47664d963d530255"
        platform="linux"
        ;;
    Linux-aarch64)
        shatool="sha256sum"
        sha256sum="5c25c6570b0155e95fce5918cb95f1ad9870df5768653afe128db822301a05a1"
        platform="linux-aarch64"
        ;;
    Darwin-*)
        shatool="shasum -a 256"
        sha256sum="89a287444b5b3e98f88a945afa50ce937b8ffd1dcc59c555ad9b1baf855298c9"
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
