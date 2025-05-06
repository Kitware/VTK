#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="lcl"
readonly ownership="Lightweight Cell Library Upstream <kwrobot@kitware.com>"
readonly subtree="viskores/thirdparty/$name/viskores$name"
readonly repo="https://gitlab.kitware.com/vtk/lcl.git"
readonly tag="for/viskores-20250206-g58a4bf3"
readonly paths="
lcl
LICENSE.md
README.md
"

extract_source () {
    git_archive
    pushd "${extractdir}/${name}-reduced"
    rm -rf lcl/testing
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
