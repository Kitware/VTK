#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="netcdf"
readonly ownership="netcdf Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/netcdf.git"
readonly tag="for/vtk-20200415-4.7.3"
readonly paths="
include/*.h
include/netcdf_meta.h.in
libdispatch/*.c
libdispatch/*.h
liblib/nc_initialize.c
libsrc/*.h
libsrc/*.c
libsrc4/*.c
libhdf5/*.c

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
    sed -i -e '/#line/d' libsrc/attr.c libsrc/ncx.c libsrc/putget.c
    # macOS takes "-e" as an extension for backup files for in-place replacement specified by -i
    # remove the backup files
    rm -f libsrc/attr.c-e libsrc/ncx.c-e libsrc/putget.c-e
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
