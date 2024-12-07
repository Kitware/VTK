#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="netcdf"
readonly ownership="netcdf Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/netcdf.git"
readonly tag="for/vtk-20231030-4.9.2"
readonly paths="
include/*.h
include/netcdf_dispatch.h.in
include/netcdf_meta.h.in
libdispatch/*.c
libdispatch/*.h
liblib/nc_initialize.c
libsrc/*.h
libsrc/*.c
libsrc4/*.c
libhdf5/*.c
libhdf5/*.h

.gitattributes
CMakeLists.vtk.txt
config.h.in
COPYRIGHT
README.md
README.kitware.md
vtk_netcdf_config.h.in
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    sed -i.bak -e '/#line/d' libsrc/attr.c libsrc/ncx.c libsrc/putget.c
    rm libsrc/attr.c.bak libsrc/ncx.c.bak libsrc/putget.c.bak
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    rm -v libdispatch/ezxml_extra.c
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
