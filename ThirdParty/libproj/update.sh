#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libproj"
readonly ownership="Proj Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/proj.git"
readonly tag="for/vtk-20230920-8.1.0"
readonly paths="
.gitattributes
ChangeLog
COPYING
CMakeLists.txt
README.md
README.kitware.md

cmake/ProjConfig.cmake
cmake/ProjTest.cmake
cmake/ProjUtilities.cmake
cmake/ProjVersion.cmake
cmake/proj_config.cmake.in

data/

include/CMakeLists.txt
include/proj/CMakeLists.txt
include/proj/*.hpp
include/proj/internal/*.hpp

src/CMakeLists.txt
src/lib_proj.cmake
src/projections/*.cpp
src/conversions/*.cpp
src/transformations/*.cpp
src/transformations/*.hpp
src/iso19111/*.cpp
src/iso19111/operation/*.cpp
src/iso19111/operation/*.hpp
src/*.c
src/*.h
src/*.cpp
src/*.hpp
src/vtk_libproj_mangle.h
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -rvf data/tests/
    rm data/Makefile.am
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
