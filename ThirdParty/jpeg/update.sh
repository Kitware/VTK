#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="jpeg"
readonly ownership="jpeg-turbo Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libjpeg-turbo.git"
readonly tag="for/vtk-20191230-2.0.3"
readonly paths="
.gitattributes
CMakeLists.vtk.txt

j*.c
j*.h
vtk_jpeg_mangle.h
jconfig.h.in
jconfigint.h.in
win/jconfig.h.in

LICENSE.md
README.ijg
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -v *-tj.c
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
