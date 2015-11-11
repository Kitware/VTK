#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="metaio"
readonly ownership="MetaIO Upstream <kwrobot@kitware.com>"
readonly subtree="Utilities/MetaIO/vtk$name"
readonly update="Utilities/MetaIO/update.sh"
readonly repo="https://github.com/Kitware/MetaIO.git"
readonly tag="master"
readonly paths="
src/*.*
src/doc
License.txt
"

readonly basehash='9db802656c5e49aaea85386f8dacf148b8e53ca9' # NEWHASH

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v src/* .
    rmdir src/
    popd
}

. "${BASH_SOURCE%/*}/../../ThirdParty/update-common.sh"
