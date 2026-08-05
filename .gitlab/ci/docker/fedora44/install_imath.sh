#!/bin/sh

set -e

readonly imath_repo="https://github.com/AcademySoftwareFoundation/Imath.git"
readonly imath_commit="v3.2.2"

readonly imath_root="$HOME/imath"
readonly imath_src="$imath_root/src"
readonly imath_build="$imath_root/build"

git clone -b "$imath_commit" "$imath_repo" "$imath_src"

mkdir -p "$imath_build"
cd "$imath_build"

cmake -GNinja "$imath_src" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_INSTALL_LIBDIR=lib64 \
    -DBUILD_TESTING:BOOL=OFF
ninja
cmake --install .

cd

rm -rf "$imath_root"
