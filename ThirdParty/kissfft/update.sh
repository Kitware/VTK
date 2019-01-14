#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="kissfft"
readonly ownership="Kissfft Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/kissfft.git"
readonly tag="for/vtk-20181025-master-g6f09c27"
readonly paths="
.gitattributes
CMakeLists.txt
COPYING
README
README.kitware.md

tools/
*.c
*.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
