# build zlib, png and jpeg
SUBDIRS(zlib png jpeg)
SUBDIR_DEPENDS(png zlib)

