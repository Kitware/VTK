#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="zfp"
readonly ownership="Zfp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/zfp.git"
readonly tag="for/vtk"
readonly paths="
.gitattributes
CMakeLists.txt

inc/
src/inline/
src/template/
src/*.c
src/*.h

API
LICENSE
README
README.kitware.md
VERSIONS
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
