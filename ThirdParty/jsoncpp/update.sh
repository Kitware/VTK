#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="jsoncpp"
readonly ownership="JsonCpp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/jsoncpp.git"
readonly tag="for/vtk"
readonly paths="
.gitattributes
LICENSE
" # We amalgamate jsoncpp

extract_source () {
    python amalgamate.py
    [ -n "$paths" ] && \
        mv -v $paths "dist"
    mv "dist" "$name-reduced"
    tar -cv "$name-reduced/" | \
        tar -C "$extractdir" -x
}

. "${BASH_SOURCE%/*}/../update-common.sh"
