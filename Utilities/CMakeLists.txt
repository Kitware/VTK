# build zlib, png and jpeg
SUBDIRS(zlib png jpeg Doxygen)
SUBDIR_DEPENDS(png zlib)

