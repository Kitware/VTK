#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="nanoflann"
readonly ownership="nanoflann Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/nanoflann.git"
readonly tag="for/vtk-20261014-2.0.1"
readonly paths="
.gitattributes
include/nanoflann.hpp

CMakeLists.vtk.txt
COPYING
README.kitware.md
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
