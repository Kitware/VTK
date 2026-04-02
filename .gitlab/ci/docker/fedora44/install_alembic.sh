#!/bin/sh

set -e

readonly alembic_repo="https://github.com/alembic/alembic.git"
readonly alembic_commit="1.8.11"

readonly alembic_root="$HOME/alembic"
readonly alembic_src="$alembic_root/src"
readonly alembic_build="$alembic_root/build"

git clone -b "$alembic_commit" "$alembic_repo" "$alembic_src"

mkdir -p "$alembic_build"
cd "$alembic_build"

# depends on Imath already being installed
cmake -GNinja "$alembic_src" \
    -DALEMBIC_ILMBASE_LINK_STATIC:BOOL="OFF" \
    -DALEMBIC_LIB_INSTALL_DIR:PATH="lib64" \
    -DALEMBIC_SHARED_LIBS:BOOL="ON" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_INSTALL_LIBDIR=lib64 \
    -DUSE_BINARIES:BOOL=OFF \
    -DUSE_TESTS:BOOL=OFF
ninja
cmake --install .

cd

rm -rf "$alembic_root"
