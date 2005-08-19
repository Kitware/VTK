PROJECT(vtkMPEG2Encode)
INCLUDE_REGULAR_EXPRESSION("^.*$")

#############################################################################
# to use this library in your code you will need to SUBDIR into this
# directory so that it gets built and then you will use the following variables
# in your CMakeLists files to get the proper include paths and libraries
SET (vtkMPEG2Encode_INCLUDE_PATH 
  "${vtkMPEG2Encode_SOURCE_DIR}"
  CACHE INTERNAL "include paths for vtkMPEG2Encode"
  )
SET (vtkMPEG2Encode_LIBRARIES vtkMPEG2Encode 
  CACHE INTERNAL "libraries for vtkMPEG2Encode")
#############################################################################

# We need ansi c-flags, especially on HP
SET(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
SET(CMAKE_REQUIRED_FLAGS ${CMAKE_ANSI_CFLAGS})

SET(srcs
  conform.c  motion.c    putbits.c  putpic.c  quantize.c  stats.c
  fdctref.c  puthdr.c   putseq.c  ratectl.c   transfrm.c
  idct.c     predict.c   putmpg.c   putvlc.c  readpic.c   writepic.c
  )

ADD_LIBRARY(vtkMPEG2Encode ${srcs})

IF (WIN32)
  IF (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DMPEG2ENCDLL)
  ELSE (BUILD_SHARED_LIBS)
    SET(MPEG2ENCSTATIC 1)
  ENDIF (BUILD_SHARED_LIBS)
ENDIF (WIN32)

CONFIGURE_FILE(${vtkMPEG2Encode_SOURCE_DIR}/.NoDartCoverage
               ${vtkMPEG2Encode_BINARY_DIR}/.NoDartCoverage)
CONFIGURE_FILE(${vtkMPEG2Encode_SOURCE_DIR}/mpeg2encDllConfig.h.in
               ${vtkMPEG2Encode_BINARY_DIR}/mpeg2encDllConfig.h)

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(vtkMPEG2Encode PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTK_INSTALL_LIB_DIR}
                  RUNTIME_DIRECTORY ${VTK_INSTALL_BIN_DIR} vtkMPEG2Encode)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
