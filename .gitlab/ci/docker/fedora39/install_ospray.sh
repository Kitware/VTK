#!/bin/sh

set -e

readonly ospray_version="2.11.0"
readonly ospray_tarball="v$ospray_version.tar.gz"
readonly ospray_sha256sum="55974e650d9b78989ee55adb81cffd8c6e39ce5d3cf0a3b3198c522bf36f6e81"

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
    -DBUILD_EMBREE_FROM_SOURCE=ON \
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
