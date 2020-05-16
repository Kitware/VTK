#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libproj"
readonly ownership="Proj Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/proj.git"
readonly tag="for/vtk-20200507-4.9.3"
readonly paths="
.gitattributes
ChangeLog
COPYING
CMakeLists.txt
README
README.kitware.md

cmake/Proj4Config.cmake
cmake/Proj4SystemInfo.cmake
cmake/Proj4Test.cmake
cmake/Proj4Utilities.cmake
cmake/Proj4Version.cmake
cmake/proj_config.cmake.in

src/CMakeLists.txt
src/lib_proj.cmake
src/PJ_*.c
src/aasincos.c
src/adjlon.c
src/bch2bps.c
src/bchgen.c
src/biveval.c
src/dmstor.c
src/emess.c
src/emess.h
src/geocent.c
src/geocent.h
src/geodesic.c
src/mk_cheby.c
src/nad_*.c
src/pj_*.c
src/pj_*.h
src/proj_*.c
src/rtodms.c
src/vector1.c
src/projects.h
src/proj_api.h
src/geodesic.h
src/vtk_libproj_mangle.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
