#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="gl2ps"
readonly ownership="gl2ps Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/gl2ps.git"
readonly tag="for/vtk-20181008-gb92efdf"
readonly paths="
gl2ps.c
gl2ps.h
vtk_gl2ps_mangle.h

.gitattributes
CMakeLists.vtk.txt
COPYING.GL2PS
COPYING.LGPL
README.kitware.md
README.txt
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
