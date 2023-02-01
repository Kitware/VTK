#!/bin/sh

set -e

readonly ospray_version="2.7.1"
readonly ospray_tarball="v$ospray_version.tar.gz"
readonly ospray_sha256sum="4e7bd8145e19541c04f5d949305f19a894d85a827f567d66ae2eb11a760a5ace"

readonly ospray_root="$HOME/ospray"

readonly ospray_src="$ospray_root/src"
readonly ospray_build="$ospray_root/build"

mkdir -p "$ospray_root" \
    "$ospray_src" "$ospray_build"
cd "$ospray_root"

echo "$ospray_sha256sum  $ospray_tarball" > ospray.sha256sum
curl -OL "https://github.com/ospray/ospray/archive/refs/tags/$ospray_tarball"
sha256sum --check ospray.sha256sum

tar -C "$ospray_src" --strip-components=1 -xf "$ospray_tarball"

cd "$ospray_build"

cmake -GNinja "$ospray_src/scripts/superbuild" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_INSTALL_LIBDIR=lib64 \
    -DBUILD_EMBREE_FROM_SOURCE=OFF \
    -DBUILD_GLFW=OFF \
    -DBUILD_OSPRAY_APPS=OFF \
    -DDEPENDENCIES_BUILD_TYPE=Release \
    -DDOWNLOAD_ISPC=ON \
    -DDOWNLOAD_TBB=OFF \
    -DINSTALL_IN_SEPARATE_DIRECTORIES=OFF
ninja
cmake --install .

cd

rm -rf "$ospray_root"
