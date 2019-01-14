#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="theora"
readonly ownership="theora Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/theora.git"
readonly tag="for/vtk-20190114-1.1.1"
readonly paths="
.gitattributes
COPYING
CMakeLists.txt
LICENSE
README.kitware.md
README

include/theora/codec.h
include/theora/theora.h
include/theora/theoradec.h
include/theora/theoraenc.h
include/theora/vtk_theora_mangle.h

lib/*.c
lib/*.h
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -v lib/cpu.* lib/encoder_disabled.c
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
