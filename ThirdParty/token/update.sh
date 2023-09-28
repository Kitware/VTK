#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="token"
readonly ownership="token Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/token/vtktoken"
readonly repo="https://gitlab.kitware.com/dcthomp/token.git"
readonly tag="for/vtk"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
    sed -i.bak -e '/import off/,/import on/d' "$extractdir/$name-reduced/.gitattributes"
    rm "$extractdir/$name-reduced/.gitattributes.bak"
}

. "${BASH_SOURCE%/*}/../../ThirdParty/update-common.sh"
