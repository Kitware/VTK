#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="token"
readonly ownership="token Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/token/vtktoken"
readonly repo="https://gitlab.kitware.com/utils/token.git"
readonly tag="for/vtk"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../../ThirdParty/update-common.sh"
