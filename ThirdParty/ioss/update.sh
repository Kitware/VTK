#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="ioss"
readonly ownership="Seacas Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/seacas.git"
readonly tag="ioss/for/vtk-20231017-v2022-10-14"
readonly paths="
packages/seacas/libraries/ioss/src/CMakeLists.vtk.txt
packages/seacas/libraries/ioss/cmake/SEACASIoss_config.h.in

packages/seacas/libraries/ioss/src/*.hpp
packages/seacas/libraries/ioss/src/*.h
packages/seacas/libraries/ioss/src/*.C

packages/seacas/libraries/ioss/src/exodus/*.h
packages/seacas/libraries/ioss/src/exodus/*.C

packages/seacas/libraries/ioss/src/catalyst/*.h
packages/seacas/libraries/ioss/src/catalyst/*.C

packages/seacas/libraries/ioss/src/cgns/*.h
packages/seacas/libraries/ioss/src/cgns/*.C

packages/seacas/libraries/ioss/src/gen_struc/*.h
packages/seacas/libraries/ioss/src/gen_struc/*.C

packages/seacas/libraries/ioss/src/generated/*.h
packages/seacas/libraries/ioss/src/generated/*.C

packages/seacas/libraries/ioss/src/heartbeat/*.h
packages/seacas/libraries/ioss/src/heartbeat/*.C

packages/seacas/libraries/ioss/src/init/*.h
packages/seacas/libraries/ioss/src/init/*.C

packages/seacas/libraries/ioss/src/transform/*.h
packages/seacas/libraries/ioss/src/transform/*.C

packages/seacas/libraries/ioss/src/.gitattributes
packages/seacas/libraries/ioss/src/COPYRIGHT
packages/seacas/libraries/ioss/src/README.kitware.md
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    mv -v packages/seacas/libraries/ioss/cmake .
    mv -v packages/seacas/libraries/ioss/src/* .
    rm -rvf packages
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
