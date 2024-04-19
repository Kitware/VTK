#!/bin/sh

set -e

if command -v podman >/dev/null 2>/dev/null; then
    docker="podman"
else
    docker="docker"
fi
readonly docker

readonly image="$1"
shift

arch="$( uname -m )"
readonly arch

version="$( grep mesa_version= install_mesa_egl.sh | cut -d\" -f2 )"
readonly version

date="$( date +%Y%m%d )"
readonly date

exec $docker run --rm "$image" tar cJf - /opt/mesa > "vtk-mesa-egl-$version-$arch-$date.tar.xz"
