#!/bin/sh

set -e
set -x

readonly vtkroot="/vtk"
readonly vtksrc="$vtkroot/src"
readonly vtkout="$vtkroot/out"

readonly container="vtk-wheel"
readonly baseimage="quay.io/pypa/manylinux2010_x86_64"

mkdir -p "$PWD/dist"

podman pull "$baseimage"
podman run -it --rm "--workdir=$vtksrc" "--name=$container" "-v=$PWD:$vtksrc:Z,ro" "-v=$PWD/dist:$vtkout:Z" "$baseimage" /bin/bash "Utilities/Maintenance/vtk_wheel_build.sh"
