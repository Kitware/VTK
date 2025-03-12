#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="fast_float"
readonly ownership="fast_float Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/fast_float.git"
readonly tag="for/vtk-20250313-8.0.2"
readonly paths="
include/fast_float/*
.gitattributes
AUTHORS
CMakeLists.vtk.txt
CONTRIBUTORS
LICENSE-MIT
README.kitware.md
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mkdir vtkfast_float
    mv -v include/fast_float/* ./vtkfast_float
    mv CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
