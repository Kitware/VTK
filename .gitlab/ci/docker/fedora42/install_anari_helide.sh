#!/bin/sh

set -e

if [ "$( uname -m )" != "x86_64" ]; then
    exit 0
fi

readonly anari_repo="https://github.com/KhronosGroup/ANARI-SDK.git"
readonly anari_commit="v0.12.1"

readonly anari_root="$HOME/ANARI-SDK"
readonly anari_src="$HOME/ANARI-SDK"
readonly anari_build="$HOME/ANARI-SDK/build"

git clone -b "$anari_commit" "$anari_repo" "$anari_src"

mkdir -p "$anari_build"
cd "$anari_build"

cmake -G "Unix Makefiles" "$anari_src" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/opt/anari \
    -DBUILD_HELIDE_DEVICE:BOOL=ON \
    -DBUILD_VIEWER:BOOL=OFF \
    -DBUILD_EXAMPLES:BOOL=OFF \
    -DBUILD_CTS:BOOL=OFF \
    -DBUILD_TESTING:BOOL=OFF
cmake --build . --parallel
cmake --install .

cd

rm -rf "$anari_root"
