#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="utf8"
readonly ownership="utf8cpp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/utfcpp.git"
readonly tag="for/vtk-20181015-2.3.4"
readonly paths="
v2_0/.gitattributes
v2_0/LICENSE
v2_0/README.kitware.md
v2_0/source/
"

extract_source () {
    git_archive
    # Everything is under an v2_0 directory; remove it.
    mv "$extractdir/$name-reduced/v2_0/"* "$extractdir/$name-reduced/"
    rmdir "$extractdir/$name-reduced/v2_0"
    # The project has an extra source subdirectory; remove it.
    mv "$extractdir/$name-reduced/source/"* "$extractdir/$name-reduced/"
    rmdir "$extractdir/$name-reduced/source"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
