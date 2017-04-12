#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="ZopeInterface"
readonly ownership="zope.interface Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/zope.git"
readonly tag="for/vtk"
readonly paths="
COPYRIGHT.txt
LICENSE.txt
README.kitware.md
README.rst
.gitattributes
src/zope/
"

extract_source () {
    # Copy over the files from Git
    git_archive

    # Remove all `tests/` subdirectories.
    pushd "$extractdir"
    find . -name "tests" -type d -exec rm -rf '{}' \;
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
