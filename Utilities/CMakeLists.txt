# build zlib, png, jpeg, and tiff, expat, Doxygen doc, freetype, ftgl

SUBDIRS(zlib png jpeg tiff expat Doxygen)

IF (VTK_USE_FREETYPE)
  SUBDIRS(freetype ftgl)
ENDIF (VTK_USE_FREETYPE)
