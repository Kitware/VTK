# source files for zlib
SOURCE_FILES(ZLIB_SRCS
adler32.c   gzio.c      inftrees.c  uncompr.c
compress.c  infblock.c  infutil.c   zutil.c
crc32.c     infcodes.c  maketree.c
deflate.c   inffast.c   
inflate.c   trees.c 
)

# for windows add the .def and .rc files to the source list
IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DZLIB_DLL)
  ENDIF(BUILD_SHARED_LIBS)
  IF(NOT UNIX)
    SOURCE_FILES(ZLIB_SRCS zlib.def zlib.rc  )
  ENDIF(NOT UNIX)
ENDIF(WIN32)
ADD_LIBRARY(zlib ZLIB_SRCS)
