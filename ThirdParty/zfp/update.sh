#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="zfp"
readonly ownership="Zfp Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/zfp.git"
readonly tag="for/vtk-20190605-0.5.5"
readonly paths="
.gitattributes
CMakeLists.vtk.txt

include/
array/
src/inline/
src/template/
src/share/
src/*.c
src/*.h

LICENSE
README.md
README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    mv -v array/zfp/* include/zfp/
    rmdir -v array/zfp/
    mv -v array/* include/
    rmdir -v array/
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
