PROJECT(VTKZLIB)
INCLUDE_REGULAR_EXPRESSION("^(vtk|deflate|inf|trees|zconf|zlib|zutil).*$")

INCLUDE_DIRECTORIES(BEFORE ${VTKZLIB_SOURCE_DIR})

# source files for zlib
SET(ZLIB_SRCS
  adler32.c  compress.c  crc32.c  deflate.c  gzio.c   inffast.c
  inflate.c  inftrees.c  trees.c  uncompr.c  zutil.c
  )

# for windows add the .def and .rc files to the source list
# if building shared libs
IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    SET(ZLIB_DLL 1)
    IF(NOT UNIX)
      IF(NOT BORLAND)
        IF(NOT MINGW)
        ENDIF(NOT MINGW)
      ENDIF(NOT BORLAND) 
    ENDIF(NOT UNIX)   
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(WIN32)

CONFIGURE_FILE(${VTKZLIB_SOURCE_DIR}/zlibDllConfig.h.in
               ${VTKZLIB_BINARY_DIR}/zlibDllConfig.h)


VTK_ADD_LIBRARY(vtkzlib ${ZLIB_SRCS})

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKZLIB_SOURCE_DIR}/zlib.h
    ${VTKZLIB_SOURCE_DIR}/zconf.h
    ${VTKZLIB_SOURCE_DIR}/vtk_zlib_mangle.h
    ${VTKZLIB_BINARY_DIR}/zlibDllConfig.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtkzlib
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
