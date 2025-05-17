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
readonly prefix="vtk"
readonly package_name="$prefix-$architecture"

cd build/install/bin
curl --header "JOB-TOKEN: $CI_JOB_TOKEN" --upload-file "$package_name.tar.gz" "$CI_API_V4_URL/projects/$CI_PROJECT_ID/packages/generic/$package_name/$version/$package_name.tar.gz"
cd ../../
