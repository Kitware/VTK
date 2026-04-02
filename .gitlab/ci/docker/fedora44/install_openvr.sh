#!/bin/sh

set -e

case "$( uname -m )" in
    x86_64)
        subdir="linux64"
        ;;
    aarch64)
        subdir="linuxarm64"
        ;;
    *)
        exit 0
        ;;
esac
readonly subdir

readonly version="1.26.7"
readonly tarball="v$version.tar.gz"
readonly sha256sum="e7391f1129db777b2754f5b017cfa356d7811a7bcaf57f09805b47c2e630a725"

readonly openvr_root="$HOME/openvr"

mkdir -p "$openvr_root"
cd "$openvr_root"

echo "$sha256sum  $tarball" > openvr.sha256sum
curl -OL "https://github.com/ValveSoftware/openvr/archive/refs/tags/v$version.tar.gz"
sha256sum --check openvr.sha256sum
tar -xf "$tarball"

install -p -m 755 "openvr-$version/bin/$subdir/libopenvr_api.so" /usr/local/lib64
install -p -m 644 "openvr-$version/headers/"*.h /usr/local/include

cd

rm -rf "$openvr_root"
