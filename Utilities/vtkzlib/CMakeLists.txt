PROJECT(VTKZLIB)
INCLUDE_REGULAR_EXPRESSION("^(vtk|deflate|inf|trees|zconf|zlib|zutil).*$")

INCLUDE_DIRECTORIES(${VTKZLIB_SOURCE_DIR})

# source files for zlib
SET(ZLIB_SRCS
  adler32.c   gzio.c      inftrees.c  uncompr.c
  compress.c  infblock.c  infutil.c   zutil.c
  crc32.c     infcodes.c 
  deflate.c   inffast.c  
  inflate.c   trees.c
  )

# for windows add the .def and .rc files to the source list if building shared libs
IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    SET(ZLIB_DLL 1)
    IF(NOT UNIX)
      IF(NOT BORLAND)
        IF(NOT MINGW)
          SET(ZLIB_SRCS ${ZLIB_SRCS} zlib.def zlib.rc  )
        ENDIF(NOT MINGW)
      ENDIF(NOT BORLAND) 
    ENDIF(NOT UNIX)   
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(WIN32)

CONFIGURE_FILE(${VTKZLIB_SOURCE_DIR}/.NoDartCoverage
  ${VTKZLIB_BINARY_DIR}/.NoDartCoverage)
CONFIGURE_FILE(${VTKZLIB_SOURCE_DIR}/zlibDllConfig.h.in
  ${VTKZLIB_BINARY_DIR}/zlibDllConfig.h)


ADD_LIBRARY(vtkzlib ${ZLIB_SRCS})

INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR} vtkzlib)
INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR}/vtkzlib .h
              zlib zconf zlibDllConfig vtk_zlib_mangle)
