#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="ogg"
readonly ownership="ogg Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/ogg.git"
readonly tag="for/vtk-20181015-1.3.3"
readonly paths="
.gitattributes
COPYING
CMakeLists.txt
README.kitware.md
README.md

src/bitwise.c
src/framing.c

include/ogg/config_types.h.in
include/ogg/ogg.h
include/ogg/os_types.h
include/ogg/vtk_ogg_mangle.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
