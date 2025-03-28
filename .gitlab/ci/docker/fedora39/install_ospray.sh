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
    -DINSTALL_DEPENDENCIES=OFF \
    -DDOWNLOAD_ISPC=ON \
    -DDOWNLOAD_TBB=OFF \
    -DINSTALL_IN_SEPARATE_DIRECTORIES=OFF
ninja
cmake --install .

cd

# See https://github.com/RenderKit/rkcommon/issues/15
echo '--- /usr/local/lib64/cmake/rkcommon-1.11.0/FindTBB.cmake.orig   2025-03-28 23:55:15.000000000 +0000
+++ /usr/local/lib64/cmake/rkcommon-1.11.0/FindTBB.cmake        2025-03-28 23:58:23.792972523 +0000
@@ -37,7 +37,8 @@
 #===============================================================================

 # We use INTERFACE libraries, which are only supported in 3.x
-cmake_minimum_required(VERSION 3.1)
+cmake_policy(PUSH)
+cmake_policy(VERSION 3.1...3.20)

 # These two are used to automatically find the root and include directories.
 set(_TBB_INCLUDE_SUBDIR "include")
@@ -482,3 +483,4 @@

 set(TBB_FOUND TRUE)
 set(TBB_INCLUDE_DIRS "${TBB_INCLUDE_DIR}")
+cmake_policy(POP)' | patch /usr/local/lib64/cmake/rkcommon-1.11.0/FindTBB.cmake -b

rm -rf "$ospray_root"
