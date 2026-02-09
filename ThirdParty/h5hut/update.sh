#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="h5hut"
readonly ownership="H5hut Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/h5hut.git"
readonly tag="for/vtk-20260125-v2.0.0rc7"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
config.h.cmake.in
COPYING
license.txt
README.kitware.md
src/include/*.h
src/include/h5core/*.h
src/h5core/*.c
src/h5core/private/*.h
src/h5core/private/*.c
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
