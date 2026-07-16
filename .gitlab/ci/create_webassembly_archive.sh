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

install_dir="$(pwd)/build/install"
readonly install_dir

# JSON type manifests (when VTK_BUILD_TYPES_JSON=ON) install under
# share/vtk<suffix>/types. They are architecture-specific (word-width types like
# vtkIdType/size_t are baked to the build's pointer width), so each archive
# bundles the manifest set produced by its own build, nested under types/. The
# consumer picks the wasm32 or wasm64 archive to match its target.
types_dir="$( ls -d "$install_dir"/share/vtk*/types 2>/dev/null | head -n1 )"
readonly types_dir

cd "$install_dir/bin"
if [ -n "$types_dir" ] && [ -d "$types_dir" ]; then
  tar -cvzf "vtk-$version-$architecture.tar.gz" \
    ./vtkWebAssembly* \
    -C "$( dirname "$types_dir" )" "$( basename "$types_dir" )"
else
  tar -cvzf "vtk-$version-$architecture.tar.gz" ./vtkWebAssembly*
fi
cd ../../
