#!/bin/sh

set -e

readonly alembic_version="1.8.5"
readonly alembic_tarball="$alembic_version.tar.gz"
readonly alembic_sha256sum="180a12f08d391cd89f021f279dbe3b5423b1db751a9898540c8059a45825c2e9"

readonly alembic_root="$HOME/alembic"

readonly alembic_src="$alembic_root/src"
readonly alembic_build="$alembic_root/build"

mkdir -p "$alembic_root" \
    "$alembic_src" "$alembic_build"
cd "$alembic_root"

echo "$alembic_sha256sum  $alembic_tarball" > alembic.sha256sum
curl -OL "https://github.com/alembic/alembic/archive/refs/tags/$alembic_tarball"
sha256sum --check alembic.sha256sum

tar -C "$alembic_src" --strip-components=1 -xf "$alembic_tarball"

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
