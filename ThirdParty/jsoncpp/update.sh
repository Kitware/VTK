#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="jsoncpp"
readonly ownership="JsonCpp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly update="ThirdParty/$name/update.sh"
readonly repo="https://gitlab.kitware.com/third-party/jsoncpp.git"
readonly tag="for/vtk"
readonly paths="
.gitattributes
LICENSE
" # We amalgamate jsoncpp

readonly basehash='f3f68fdbe67acd1a18ac2f93085d43069c3b569f' # NEWHASH

extract_source () {
    python amalgamate.py
    [ -n "$paths" ] && \
        mv -v $paths "dist"
    mv "dist" "$name-reduced"
    tar -cv "$name-reduced/" | \
        tar -C "$extractdir" -x
}

. "$( dirname "${BASH_SOURCE[0]}" )/../update-common.sh"
