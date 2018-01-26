#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="png"
readonly ownership="Libpng Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/png.git"
readonly tag="for/vtk-old"
readonly paths="
.gitattributes
CMakeLists.vtk.txt

scripts/dfn.awk
scripts/options.awk
scripts/pnglibconf.dfa
scripts/pnglibconf.mak
pngusr.dfa

CHANGES
LICENSE
README
README.kitware.md

png.c
pngerror.c
pngget.c
pngmem.c
pngpread.c
pngread.c
pngrio.c
pngrtran.c
pngrutil.c
pngset.c
pngtrans.c
pngwio.c
pngwrite.c
pngwtran.c
pngwutil.c

png.h
pngconf.h
pngdebug.h
pnginfo.h
pngpriv.h
pngstruct.h
vtk_png_mangle.h
vtkpngConfig.h.in
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    CPPFLAGS="-I${BASH_SOURCE%/*}/../zlib/vtkzlib" make -f scripts/pnglibconf.mak
    sed -i -e '/PNG_ZLIB_VERNUM/s/0x.*/0/' pnglibconf.h
    rm -rvf scripts pngusr.dfa pnglibconf.dfn pnglibconf.pre
    mv -v CMakeLists.vtk.txt CMakeLists.txt
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
