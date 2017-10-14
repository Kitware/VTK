#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="wslink"
readonly ownership="wslink Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://github.com/Kitware/wslink.git"
readonly tag="master"
readonly paths="
LICENSE
README.md
python/src/wslink/*.py
"

extract_source () {
    # Copy over the files from Git
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
