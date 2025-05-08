#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="lodepng"
readonly ownership="LodePNG Upstream <kwrobot@kitware.com>"
readonly subtree="viskores/thirdparty/$name/viskores$name"
readonly repo="https://gitlab.kitware.com/third-party/$name.git"
readonly tag="for/viskores-20250206-g690034b"
readonly paths="
LICENSE
lodepng.cpp
lodepng.h
README.md
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
