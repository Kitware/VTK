#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="cli11"
readonly ownership="CLI11 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/cli11.git"
readonly tag="for/vtk-2021-05-13-v1.9.1"
readonly paths="
include/CLI/*.hpp
LICENSE
README.md
README.kitware.md
CMakeLists.vtk.txt
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v include/* .
    mv CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
