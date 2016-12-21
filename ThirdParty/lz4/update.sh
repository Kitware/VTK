#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="lz4"
readonly ownership="lz4 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/lz4.git"
readonly tag="for/vtk"

readonly paths="
lib/LICENSE
lib/README.md
lib/*.h
lib/*.c
README.md
CMakeLists.txt

.gitattributes
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
