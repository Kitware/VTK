#!/bin/sh

set -e

readonly version="0.2.15-background-init"
readonly build_date="20210523.0"

case "$( uname -s )-$(uname -m)" in
    Linux-x86_64)
        shatool="sha256sum"
        sha256sum="e735afaac1ac3655a77104296535476a9485f571c67d22a1381a3f1b32c73172"
        platform="x86_64-unknown-linux-musl"
        ;;
    Darwin-x86_64|Darwin-arm64)
        shatool="shasum -a 256"
        sha256sum="9fca55e204784eb1766533205a93fa30aadf9d60956bc899dce142bc4806f86b"
        platform="universal-apple-darwin"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="sccache-v$version-$platform"

readonly url="https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/sccache/v$version-$build_date/"

cd .gitlab

echo "$sha256sum  $filename" > sccache.sha256sum
curl -OL "$url/$filename"
$shatool --check sccache.sha256sum
mv "$filename" sccache
chmod +x sccache
