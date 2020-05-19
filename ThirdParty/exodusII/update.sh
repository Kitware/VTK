#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="exodusII"
readonly ownership="Seacas Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/seacas.git"
readonly tag="for/vtk-20200507-7.24f-v2019-12-18"
readonly paths="
packages/seacas/libraries/exodus/CMakeLists.vtk.txt
packages/seacas/libraries/exodus/cmake/exodus_config.h.in
packages/seacas/libraries/exodus/include/exodusII.h
packages/seacas/libraries/exodus/include/exodusII_cfg.h.in
packages/seacas/libraries/exodus/include/exodusII_int.h
packages/seacas/libraries/exodus/include/vtk_exodusII_mangle.h
packages/seacas/libraries/exodus/src/*.c
packages/seacas/libraries/exodus/src/deprecated/*.c

packages/seacas/libraries/exodus/.gitattributes
packages/seacas/libraries/exodus/COPYRIGHT
packages/seacas/libraries/exodus/README
packages/seacas/libraries/exodus/README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v packages/seacas/libraries/exodus/* .
    rm -rvf packages
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
