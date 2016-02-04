#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="KWIML"
readonly ownership="KWIML Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/KWIML/vtkkwiml"
readonly repo="https://github.com/Kitware/KWIML.git"
readonly tag="master"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../../ThirdParty/update-common.sh"
