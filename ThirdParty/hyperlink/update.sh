#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="hyperlink"
readonly ownership="hyperlink Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/hyperlink.git"
readonly tag="for/vtk"
readonly paths="
LICENSE
README.kitware.md
README.md
.gitattributes
hyperlink/*.py
"
extract_source () {
    # Copy over the files from Git
    git_archive

}
. "${BASH_SOURCE%/*}/../update-common.sh"
