#!/bin/sh

set -e

readonly version="0.12.1"

case "$( uname -s )-$( uname -m )" in
    Linux-x86_64)
        shatool="sha256sum"
        sha256sum="90b2f223fb69d19db49e117da601f64978593417988530aa733d456141b4bcbb"
        platform="x86_64-unknown-linux-gnu"
        ;;
    Darwin-arm64)
        shatool="shasum -a 256"
        sha256sum="77d2906988e8074fd43f2f329ec452ebbf9b0c257ba1c66451c71de70a6baf42"
        platform="aarch64-apple-darwin"
        ;;
    Darwin-x86_64)
        shatool="shasum -a 256"
        sha256sum="69d9f9a00337f25a50dcb13882052da08b8469bac11091c98c5694c3c6721467"
        platform="x86_64-apple-darwin"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="uv-$platform"
readonly tarball="$filename.tar.gz"

readonly url="https://github.com/astral-sh/uv/releases/download/$version/$tarball"

cd .gitlab

echo "$sha256sum  $tarball" > uv.sha256sum
curl -OL "$url"
$shatool --check uv.sha256sum
tar xzf "$tarball"
mv "$filename/uv" uv
rm -rf "$filename" "$tarball" uv.sha256sum
