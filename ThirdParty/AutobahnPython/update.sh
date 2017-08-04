#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="AutobahnPython"
readonly ownership="AutobahnPython Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtkAutobahn"
readonly repo="https://gitlab.kitware.com/third-party/autobahn-python.git"
readonly tag="for/vtk"
readonly paths="
LICENSE
README.kitware.md
README.rst
.gitattributes
autobahn
"

extract_source () {
    # Copy over the files from Git
    git_archive

    # Remove all `test/` subdirectories.
    pushd "$extractdir"
    find . -name "test" -type d -exec rm -rf '{}' \;
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
