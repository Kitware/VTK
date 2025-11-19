#!/bin/sh

set -e

if [ "$( uname -m )" != "x86_64" ]; then
    exit 0
fi

readonly ospray_version="3.2.0"
readonly ospray_tarball="v$ospray_version.tar.gz"
readonly ospray_sha256sum="2c8108df2950bc5d1bc2a62f74629233dbe4f36e3f6a8ea032907d4a3fdc6750"

readonly ospray_root="$HOME/ospray"

readonly ospray_src="$ospray_root/src"
readonly ospray_build="$ospray_root/build"

mkdir -p "$ospray_root" \
    "$ospray_src" "$ospray_build"
cd "$ospray_root"

echo "$ospray_sha256sum  $ospray_tarball" > ospray.sha256sum
curl -OL "https://github.com/RenderKit/ospray/archive/refs/tags/$ospray_tarball"
sha256sum --check ospray.sha256sum

tar -C "$ospray_src" --strip-components=1 -xf "$ospray_tarball"

cd "$ospray_build"

cmake -GNinja "$ospray_src/scripts/superbuild" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_INSTALL_LIBDIR=lib64 \
    -DBUILD_EMBREE_FROM_SOURCE=ON \
    -DBUILD_OIDN_FROM_SOURCE=ON \
    -DBUILD_OPENVKL_FROM_SOURCE=ON \
    -DBUILD_GLFW=OFF \
    -DBUILD_OSPRAY_APPS=OFF \
    -DDEPENDENCIES_BUILD_TYPE=Release \
    -DINSTALL_DEPENDENCIES=OFF \
    -DDOWNLOAD_ISPC=ON \
    -DDOWNLOAD_TBB=OFF \
    -DINSTALL_IN_SEPARATE_DIRECTORIES=OFF
ninja
cmake --install .

cd

rm -rf "$ospray_root"
