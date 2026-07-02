#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="hdf5"
readonly ownership="HDF Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/hdf5.git"
readonly tag="for/vtk-20260701-2.1.1"
readonly paths="
CMakeBuildOptions.cmake
CMakeFilters.cmake
CMakeInstallation.cmake
CMakeLists.txt
CMakePlugins.cmake
UserMacros.cmake

src/
hl/CMakeLists.txt
hl/src/

config/lt_vers.am
config/clang-warnings
config/gnu-warnings
config/intel-warnings
config/ConversionTests.c
config/HDF5Macros.cmake
config/HDFMacros.cmake
config/cmake/HDF5PluginMacros.cmake
config/cmake/HDFLibMacros.cmake
config/cmake/Findlibaec.cmake
config/flags/
config/HDFMacros.cmake
config/HDFTests.c

config/ConfigureChecks.cmake

.gitattributes
ACKNOWLEDGMENTS
LICENSE
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    find . -name Makefile.am -delete
    find . -name Makefile.in -delete
    find . -name "*.lnt" -delete
    rm -v src/.indent.pro
    # Add missing newline at EOF.
    echo >> config/intel-warnings/general
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
