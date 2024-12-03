#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="jsoncpp"
readonly ownership="JsonCpp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/jsoncpp.git"
readonly tag="for/vtk-20241206-1.9.6"
readonly paths="
.gitattributes
CMakeLists.vtk.txt
README.kitware.md
LICENSE
" # We amalgamate jsoncpp

extract_source () {
    python3 amalgamate.py
    [ -n "$paths" ] && \
        mv -v $paths "dist"
    mv -v "json/vtkjsoncpp_config.h.in" "dist/json"
    mv -v "dist/CMakeLists.vtk.txt" "dist/CMakeLists.txt"
    mv -v "dist" "$name-reduced"
    tar -cv "$name-reduced/" | \
        tar -C "$extractdir" -x
}

. "${BASH_SOURCE%/*}/../update-common.sh"
