#!/bin/sh

set -e

readonly rkcommon_version="1.5.1"
readonly rkcommon_tarball="v$rkcommon_version.tar.gz"
readonly rkcommon_sha256sum="27dc42796aaa4ea4a6322f14ad64a46e83f42724c20c0f7b61d069ac91310295"

readonly embree_version="3.12.2"
readonly embree_tarball="v$embree_version.tar.gz"
readonly embree_sha256sum="22a527622497e07970e733f753cc9c10b2bd82c3b17964e4f71a5fd2cdfca210"

readonly openvkl_version="0.12.1"
readonly openvkl_tarball="v$openvkl_version.tar.gz"
readonly openvkl_sha256sum="58ce13fe09699a9966b665c89fac0fca96a1155606889e11ec8f0fc34834e25a"

readonly ospray_version="2.4.0"
readonly ospray_tarball="v$ospray_version.tar.gz"
readonly ospray_sha256sum="5eaf7409b08147cbeaf087dbf4b3887c15ffeeaa9cfd16dae3ee85504d9014c2"

readonly ospray_root="$HOME/ospray"

readonly rkcommon_src="$ospray_root/rkcommon/src"
readonly rkcommon_build="$ospray_root/rkcommon/build"
readonly embree_src="$ospray_root/embree/src"
readonly embree_build="$ospray_root/embree/build"
readonly openvkl_src="$ospray_root/openvkl/src"
readonly openvkl_build="$ospray_root/openvkl/build"
readonly ospray_src="$ospray_root/src"
readonly ospray_build="$ospray_root/build"

dnf install -y --setopt=install_weak_deps=False \
    ispc
dnf clean all

mkdir -p "$ospray_root" \
    "$rkcommon_src" "$rkcommon_build" \
    "$embree_src" "$embree_build" \
    "$openvkl_src" "$openvkl_build" \
    "$ospray_src" "$ospray_build"
cd "$ospray_root"

(
    echo "$rkcommon_sha256sum  $rkcommon_tarball"
    echo "$embree_sha256sum  $embree_tarball"
    echo "$openvkl_sha256sum  $openvkl_tarball"
    echo "$ospray_sha256sum  $ospray_tarball"
) > ospray.sha256sum
curl -OL "https://github.com/ospray/rkcommon/archive/refs/tags/$rkcommon_tarball"
curl -OL "https://github.com/embree/embree/archive/$embree_tarball"
curl -OL "https://github.com/openvkl/openvkl/archive/$openvkl_tarball"
curl -OL "https://github.com/ospray/ospray/archive/refs/tags/$ospray_tarball"
sha256sum --check ospray.sha256sum

tar -C "$rkcommon_src" --strip-components=1 -xf "$rkcommon_tarball"
tar -C "$embree_src" --strip-components=1 -xf "$embree_tarball"
tar -C "$openvkl_src" --strip-components=1 -xf "$openvkl_tarball"
tar -C "$ospray_src" --strip-components=1 -xf "$ospray_tarball"

cd "$rkcommon_build"

cmake -GNinja "$rkcommon_src" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
ninja
ninja install

cd "$embree_build"

cmake -GNinja "$embree_src" \
    -DBUILD_TESTING=OFF \
    -DBUILD_SHARED_LIBS=ON \
    -DEMBREE_TUTORIALS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
ninja
ninja install

cd "$openvkl_build"

cmake -GNinja "$openvkl_src" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
ninja
ninja install

cd "$ospray_build"

cmake -GNinja "$ospray_src" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
ninja
ninja install

cd

rm -rf "$ospray_root"
