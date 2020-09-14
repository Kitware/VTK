#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="rapidjson"
readonly ownership="RapidJSON Upstream <kwrobot@kitware.com>"
readonly subtree="thirdparty/$name/fides$name"
readonly repo="https://gitlab.kitware.com/third-party/rapidjson.git"
readonly tag="for/fides-20200811-master"
readonly paths="
.gitattributes
include
license.txt
readme.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv include/rapidjson include/fidesrapidjson
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
