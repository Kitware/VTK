#!/bin/sh

set -e

readonly version="1.16.8"
readonly tarball="v$version.tar.gz"
readonly sha256sum="387c98c0540f66595c4594e5f3340a1095dd90e954ff14fd5d89cc849ba32d1b"

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
