#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="exprtk"
readonly ownership="ExprTk Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/exprtk.git"
readonly tag="for/vtk-20210816-2.71"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
exprtk.hpp
License.txt
readme.txt
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
