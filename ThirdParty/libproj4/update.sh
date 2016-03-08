#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libproj4"
readonly ownership="Proj Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/proj.git"
readonly tag="for/vtk"
readonly paths="
AUTHORS
ChangeLog
cmake/*.cmake
cmake/*.cmake.in
CMakeLists.txt
COPYING
src/CMakeLists.txt
src/proj.def
src/*.c
src/*.cmake
src/*.h
NEWS
README
.gitattributes
.gitignore
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"

