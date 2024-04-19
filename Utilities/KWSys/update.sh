#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="KWSys"
readonly ownership="KWSys Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/KWSys/vtksys"
readonly repo="https://gitlab.kitware.com/utils/kwsys.git"
readonly tag="master"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
    sed -i.bak -e '/import off/,/import on/d' "$extractdir/$name-reduced/.gitattributes"
    rm "$extractdir/$name-reduced/.gitattributes.bak"
}

. "${BASH_SOURCE%/*}/../../ThirdParty/update-common.sh"
