#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="fmt"
readonly ownership="{fmt} Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/fmt.git"
readonly tag="for/vtk-20231005-9.1.0"
readonly paths="
.gitattributes
include/fmt/*.h
src/format.cc
src/os.cc
support/cmake/cxx14.cmake
CMakeLists.vtk.txt
LICENSE.rst
README.kitware.md
README.rst
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v include/fmt vtkfmt
    mv CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
