#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libharu"
readonly ownership="Libharu Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libharu.git"
readonly tag="for/vtk"
readonly paths="
.gitignore
.gitattributes
INSTALL
CMakeLists.txt
CHANGES

README.kitware.md
README
README_cmake

cmake/modules/*.cmake

src/.gitignore
src/CMakeLists.txt
src/*.c
src/*.h

include/hpdf_config.h.cmake
include/.gitignore
include/*.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
