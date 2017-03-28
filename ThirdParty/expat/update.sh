#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="expat"
readonly ownership="Expat Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/expat.git"
readonly tag="for/vtk"
readonly paths="
expat/.gitattributes
expat/CMakeLists.txt
expat/COPYING
expat/ConfigureChecks.cmake
expat/README
expat/expat_config.h.cmake
expat/lib/*.c
expat/lib/*.h
"

extract_source () {
    git_archive
    # Everything is under an extra expat/ directory; remove it.
    mv "$extractdir/$name-reduced/expat/"* "$extractdir/$name-reduced/"
    rmdir "$extractdir/$name-reduced/expat"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
