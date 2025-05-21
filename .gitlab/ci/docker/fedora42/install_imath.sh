#!/bin/sh

set -e

readonly imath_version="3.1.9"
readonly imath_tarball="v$imath_version.tar.gz"
readonly imath_sha256sum="f1d8aacd46afed958babfced3190d2d3c8209b66da451f556abd6da94c165cf3"

readonly imath_root="$HOME/imath"

readonly imath_src="$imath_root/src"
readonly imath_build="$imath_root/build"

mkdir -p "$imath_root" \
    "$imath_src" "$imath_build"
cd "$imath_root"

echo "$imath_sha256sum  $imath_tarball" > imath.sha256sum
curl -OL "https://github.com/AcademySoftwareFoundation/Imath/archive/refs/tags/$imath_tarball"
sha256sum --check imath.sha256sum

tar -C "$imath_src" --strip-components=1 -xf "$imath_tarball"

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
