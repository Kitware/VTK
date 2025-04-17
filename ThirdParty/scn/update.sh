#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="scn"
readonly ownership="scnlib Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/scnlib.git"
readonly tag="for/vtk-20250416-4.0.1"
readonly paths="
.gitattributes
include/scn/*.h
src/scn/*.h
src/scn/*.cpp
CMakeLists.vtk.txt
LICENSE
LICENSE.nanorange
README.kitware.md
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v include/scn include/vtkscn
    mv -v src/scn src/vtkscn
    mv CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
