PROJECT(VTKZLIB)

# source files for zlib
SOURCE_FILES(ZLIB_SRCS
adler32.c   gzio.c      inftrees.c  uncompr.c
compress.c  infblock.c  infutil.c   zutil.c
crc32.c     infcodes.c  
deflate.c   inffast.c   
inflate.c   trees.c 
)

CONFIGURE_FILE(
${VTKZLIB_SOURCE_DIR}/.NoDartCoverage 
${VTKZLIB_BINARY_DIR}/.NoDartCoverage)

# for windows add the .def and .rc files to the source list
IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DZLIB_DLL)
  ENDIF(BUILD_SHARED_LIBS)
  IF(NOT UNIX)
    IF(NOT BORLAND)
       SOURCE_FILES(ZLIB_SRCS zlib.def zlib.rc  )
    ENDIF(NOT BORLAND)  
  ENDIF(NOT UNIX)
    
ENDIF(WIN32)

ADD_LIBRARY(vtkzlib ZLIB_SRCS)

INSTALL_TARGETS(/lib/vtk vtkzlib)
