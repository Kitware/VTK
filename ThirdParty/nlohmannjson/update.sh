#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="nlohmannjson"
readonly ownership="nlohmann json Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/nlohmann_json.git"
readonly tag="for/vtk-20211106-3.10.4"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
include/
LICENSE.MIT
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    mv -v include/nlohmann include/vtknlohmann
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
