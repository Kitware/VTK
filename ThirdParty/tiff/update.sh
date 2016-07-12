#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="tiff"
readonly ownership="Tiff Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/tiff.git"
readonly tag="for/vtk"
readonly paths="
.gitattributes
CMakeLists.txt
configure.ac

libtiff/CMakeLists.txt
libtiff/mkg3states.c
libtiff/mkspans.c
libtiff/SConstruct
libtiff/t4.h
libtiff/tif_aux.c
libtiff/tif_close.c
libtiff/tif_codec.c
libtiff/tif_color.c
libtiff/tif_compress.c
libtiff/tif_config.h.cmake.in
libtiff/tif_config.h.in
libtiff/tif_config.h-vms
libtiff/tif_config.vc.h
libtiff/tif_config.wince.h
libtiff/tif_dir.c
libtiff/tif_dir.h
libtiff/tif_dirinfo.c
libtiff/tif_dirread.c
libtiff/tif_dirwrite.c
libtiff/tif_dumpmode.c
libtiff/tif_error.c
libtiff/tif_extension.c
libtiff/tif_fax3.c
libtiff/tif_fax3.h
libtiff/tif_fax3.c
libtiff/tif_fax3.h
libtiff/tif_fax3sm.c
libtiff/tiffconf.h.cmake.in
libtiff/tiffconf.h.in
libtiff/tiffconf.h-vms
libtiff/tiffconf.vc.h
libtiff/tiffconf.wince.h
libtiff/tiff.h
libtiff/tiffio.h
libtiff/tiffiop.h
libtiff/tif_flush.c
libtiff/tiffvers.h
libtiff/tiffvers.h.in
libtiff/tif_getimage.c
libtiff/tif_jbig.c
libtiff/tif_jpeg_12.c
libtiff/tif_jpeg.c
libtiff/tif_luv.c
libtiff/tif_lzma.c
libtiff/tif_lzw.c
libtiff/tif_next.c
libtiff/tif_ojpeg.c
libtiff/tif_open.c
libtiff/tif_packbits.c
libtiff/tif_pixarlog.c
libtiff/tif_predict.c
libtiff/tif_predict.h
libtiff/tif_print.c
libtiff/tif_read.c

libtiff/tif_strip.c
libtiff/tif_swab.c
libtiff/tif_thunder.c
libtiff/tif_tile.c
libtiff/tif_unix.c
libtiff/tif_version.c
libtiff/tif_vms.c
libtiff/tif_warning.c
libtiff/tif_win32.c
libtiff/tif_wince.c
libtiff/tif_write.c
libtiff/tif_zip.c
libtiff/uvcode.h

libtiff/vtk_tiff_mangle.h
libtiff/libtiff.def
libtiff/libtiff.map
libtiff/libtiffxx.map

port/CMakeLists.txt
port/dummy.c
port/getopt.c
port/lfind.c
port/libport.h
port/Makefile.am
port/Makefile.in
port/Makefile.vc
port/snprintf.c
port/strcasecmp.c
port/strtoul.c
port/strtoull.c

ChangeLog
COMMITTERS
README
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
