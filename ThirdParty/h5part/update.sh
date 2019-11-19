#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="h5part"
readonly ownership="H5part Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/h5part.git"
readonly tag="for/vtk-20191119-1.6.6"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
COPYING
README.kitware.md
README
src/*.c
src/*.h
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
