#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="verdict"
readonly ownership="Verdict Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/verdict.git"
readonly tag="for/vtk-20231029-1.4.0"
readonly paths="
.gitattributes
V_*
verdict.h
verdict_config.h.in
verdict_defines.hpp
VerdictVector.*
CMakeLists.vtk.txt
LICENSE
README.kitware.md
README.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
