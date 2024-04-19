#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="cgns"
readonly ownership="cgns Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/cgns.git"
readonly tag="for/vtk-20230531-4.2.0"

readonly paths="
.gitattributes
CMakeLists.txt
README.md
README.kitware.md
license.txt

src/CMakeLists.txt
src/cgns_error.c
src/cgns_header.h
src/cgns_internals.c
src/cgns_io.c
src/cgns_io.h
src/vtk_cgns_mangle.h
src/cgnsconfig.h.in
src/cgnslib.c
src/cgnslib.h
src/cgnstypes.h.in
src/cg_hash_types.h.in
src/cg_hashmap.h
src/cg_hashmap.c
src/pcgnslib.c
src/pcgnslib.h
src/adf/ADF.h
src/adf/ADF_interface.c
src/adf/ADF_internals.*
src/adfh/ADF.h
src/adfh/ADFH.c
src/adfh/ADFH.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
