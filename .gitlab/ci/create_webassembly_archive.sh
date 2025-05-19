#!/bin/sh

set -e
set -x

# Select the wasm architecture for the package.
readonly architecture="${1}"
# Compute the version number for the upload.
major="$( sed -ne '/set.VTK_MAJOR_VERSION/s/.* \([0-9]*\))/\1/p' CMake/vtkVersion.cmake )"
readonly major
minor="$( sed -ne '/set.VTK_MINOR_VERSION/s/.* \([0-9]*\))/\1/p' CMake/vtkVersion.cmake )"
readonly minor
build="$( sed -ne '/set.VTK_BUILD_VERSION/s/.* \([0-9]*\))/\1/p' CMake/vtkVersion.cmake )"
readonly build
version="$major.$minor.$build"
readonly version

cd build/install/bin
tar -cvzf "vtk-$version-$architecture.tar.gz" ./vtkWebAssemblyInterface.*
cd ../../
