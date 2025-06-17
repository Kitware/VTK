#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="viskores"
readonly ownership="Viskores upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name/$name"
readonly repo="https://github.com/Viskores/viskores.git"
readonly tag="521f3b72aabe0bf37e9972975700df27adbbae71" # Commit just past v1.0.0
readonly paths="
CMake/testing/ViskoresTestWrappers.cmake
CMake/FindPyexpander.cmake
CMake/InstantiationTemplate.cxx.in
CMake/ViskoresBuildType.cmake
CMake/ViskoresCompilerFlags.cmake
CMake/ViskoresConfig.cmake.in
CMake/ViskoresCPUVectorization.cmake
CMake/ViskoresDetectCUDAVersion.cu
CMake/ViskoresDetermineVersion.cmake
CMake/ViskoresDeviceAdapters.cmake
CMake/ViskoresDIYUtils.cmake
CMake/ViskoresExportHeaderTemplate.h.in
CMake/ViskoresInstallCMakePackage.cmake
CMake/ViskoresModules.cmake
CMake/ViskoresRenderingContexts.cmake
CMake/ViskoresWrappers.cmake
CMakeLists.txt
config/
CONTRIBUTING.md
CTestConfig.cmake
CTestCustom.cmake.in
examples/CMakeLists.txt
LICENSE.txt
README.md
Utilities/Git/Git.cmake
version.txt
viskores/
viskoresstd/
"


extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    # This module which is not used by neither VTK or ParaView has very long
    # paths that can over the limit of characters for Windows NTFS paths.
    rm -rf viskores/filter/scalar_topology
    sed -i 's/^.*FindTBB.cmake.*$/# XXX(kitware): removed by VTK: &/' CMake/ViskoresInstallCMakePackage.cmake
    sed -i 's/include(ViskoresCMakeBackports)$/# XXX(kitware): removed by VTK: &/' CMake/ViskoresWrappers.cmake
    sed -i 'H;1h;$!d;x;s/install(\n[^(]*FindMPI.cmake[^)]*)/# XXX(kitware): removed by VTK\n/g' CMakeLists.txt
    sed -i 's/CACHE STRING/CACHE INTERNAL/g' CMake/ViskoresModules.cmake CMake/ViskoresCPUVectorization.cmake
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
