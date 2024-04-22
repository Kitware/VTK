#!/bin/sh

set -e

readonly optix_version="6.0.0-linux64-25650775"
readonly optix_tarball="NVIDIA-OptiX-SDK-$optix_version.tar.gz"
readonly optix_sha256sum="cdd9ee5a7cb72bfcbca4d8b5ef43ae86fe43bfa82f74776801ce097b6eb7e47c"

readonly mdl_version="314800.830"
readonly mdl_tarball="mdl-sdk-$mdl_version.tar.bz2"
readonly mdl_sha256sum="ed9b9ed20977bf68554a01391311b0f7f1854106313a11ace950aea49ba43766"

readonly visrtx_version="0.1.6"
readonly visrtx_tarball="v$visrtx_version.tar.gz"
readonly visrtx_sha256sum="4ee5405a5292aa8aa3c4a0b2483ef9d43b03480664d6a466d8425a71ff65e07d"

readonly visrtx_root="$HOME/optix"
readonly visrtx_src="$visrtx_root/src"
readonly visrtx_build="$visrtx_root/build"

mkdir -p "$visrtx_root" \
    "$visrtx_src" "$visrtx_build"
cd "$visrtx_root"

(
    echo "$optix_sha256sum  $optix_tarball"
    echo "$mdl_sha256sum  $mdl_tarball"
    echo "$visrtx_sha256sum  $visrtx_tarball"
) > visrtx.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/internal/$optix_tarball"
curl -OL "https://www.paraview.org/files/dependencies/internal/$mdl_tarball"
curl -OL "https://github.com/NVIDIA/VisRTX/archive/refs/tags/$visrtx_tarball"
sha256sum --check visrtx.sha256sum

tar -C "/usr/local" "$optix_tarball" lib64 include
tar -C "/usr/local" "$mdl_tarball" lib64 include
tar -C "$visrtx_src" --strip-components=1 -xf "$visrtx_tarball"

cd "$visrtx_build"

cmake -GNinja "$visrtx_src" \
    -DBUILD_TESTING=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
ninja
ninja install

cd

rm -rf "$visrtx_root"
