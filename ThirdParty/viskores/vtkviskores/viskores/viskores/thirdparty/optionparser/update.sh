#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="optionparser"
readonly ownership="Lean Mean C++ Option Parser Upstream <kwrobot@kitware.com>"
readonly subtree="viskores/thirdparty/$name/viskores$name"
readonly repo="https://gitlab.kitware.com/third-party/$name.git"
readonly tag="for/viskores"
readonly paths="
src/optionparser.h
"

extract_source () {
    git_archive

    # The archive has sources in a src directory, but it is a header-only
    # library. Just put the headers in the base directory for easier include.
    mv "${extractdir}/${name}-reduced/src"/* "${extractdir}/${name}-reduced"
    rmdir "${extractdir}/${name}-reduced/src"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
