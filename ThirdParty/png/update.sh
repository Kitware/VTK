#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="png"
readonly ownership="Libpng Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/png.git"
readonly tag="for/vtk"
readonly paths="
.gitattributes
CMakeLists.txt
vtkpngConfig.h.in
vtk_png_mangle.h

ANNOUNCE
CHANGES
KNOWNBUG
LICENSE
README
TODO
Y2KINFO

png.c
pngconf.h
pngerror.c
pnggccrd.c
pngget.c
png.h
pngmem.c
pngpread.c
pngread.c
pngrio.c
pngrtran.c
pngrutil.c
pngset.c
pngtest.c
pngtrans.c
pngvcrd.c
pngwio.c
pngwrite.c
pngwtran.c
pngwutil.c
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
