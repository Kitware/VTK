#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="pegtl"
readonly ownership="PEGTL Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/pegtl.git"
readonly tag="for/vtk-20191226-2.8.1"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
include/
LICENSE
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
