#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="glew"
readonly ownership="glew Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/glew.git"
readonly tag="for/vtk-20190426-ge544f8c"
readonly paths="
auto
config/version
include/GL/vtk_glew_mangle.h

.gitattributes
CMakeLists.txt
LICENSE.txt
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mkdir -v src
    pushd "auto"
    make PYTHON=python2
    popd
    rm -rvf auto config
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
