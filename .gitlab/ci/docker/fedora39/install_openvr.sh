#!/bin/sh

set -e

readonly version="1.23.7"
readonly tarball="v$version.tar.gz"
readonly sha256sum="cbe2afbfc9ed9c6c5ed7df7929f9b1f5ecfd858b849b377005d4881b72b910b3"

readonly openvr_root="$HOME/openvr"

mkdir -p "$openvr_root"
cd "$openvr_root"

echo "$sha256sum  $tarball" > openvr.sha256sum
curl -OL "https://github.com/ValveSoftware/openvr/archive/refs/tags/v$version.tar.gz"
sha256sum --check openvr.sha256sum
tar -xf "$tarball"

install -p -m 755 "openvr-$version/bin/linux64/libopenvr_api.so" /usr/local/lib64
install -p -m 644 "openvr-$version/headers/"*.h /usr/local/include

cd

rm -rf "$openvr_root"
