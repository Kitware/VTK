#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="tiff"
readonly ownership="Tiff Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/tiff.git"
readonly tag="for/vtk-20191230-4.1.0"
readonly paths="
.gitattributes
CMakeLists.txt
ChangeLog
COPYRIGHT
README.md
README.kitware.md

libtiff/CMakeLists.txt
libtiff/libtiff.def
libtiff/libtiff.map
libtiff/*.c
libtiff/*.h
libtiff/tif_config.h.cmake.in
libtiff/tiffconf.h.cmake.in

port/CMakeLists.txt
port/dummy.c
port/libport.h
port/snprintf.c
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -vf libtiff/*.vc.h libtiff/*.wince.h
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
