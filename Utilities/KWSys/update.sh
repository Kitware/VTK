#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="KWSys"
readonly ownership="KWSys Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/KWSys/vtksys"
readonly repo="http://public.kitware.com/KWSys.git"
readonly tag="master"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../../ThirdParty/update-common.sh"
