#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="txaio"
readonly ownership="txaio Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/txaio.git"
readonly tag="for/vtk"
readonly paths="
LICENSE
README.kitware.md
README.rst
.gitattributes
txaio/
"
extract_source () {
    # Copy over the files from Git
    git_archive

}
. "${BASH_SOURCE%/*}/../update-common.sh"
