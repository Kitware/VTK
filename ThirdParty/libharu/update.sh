#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libharu"
readonly ownership="Libharu Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libharu.git"
readonly tag="for/vtk-20250317-2.4.5"
readonly paths="
.gitattributes
CMakeLists.txt
CHANGES
LICENSE

README.kitware.md
README.md

src/CMakeLists.txt
src/*.c
src/*.h

include/hpdf_config.h.cmake
include/*.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
