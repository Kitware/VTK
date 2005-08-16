PROJECT(vtkMPEG2Encode)

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
  vtkMPEG2WriterHelper.cxx
  )

IF(NOT VTKMPEG2_INSTALL_BIN_DIR)
  SET(VTKMPEG2_INSTALL_BIN_DIR /bin)
ENDIF(NOT VTKMPEG2_INSTALL_BIN_DIR)
IF(NOT VTKMPEG2_INSTALL_LIB_DIR)
  SET(VTKMPEG2_INSTALL_LIB_DIR /lib)
ENDIF(NOT VTKMPEG2_INSTALL_LIB_DIR)

# Let parent project set VTKMPEG2_INSTALL_NO_DEVELOPMENT or
# VTKMPEG2_INSTALL_NO_RUNTIME to remove components from the installation.
SET(VTKMPEG2_INSTALL_NO_LIBRARIES)
IF(BUILD_SHARED_LIBS)
  IF(VTKMPEG2_INSTALL_NO_RUNTIME AND VTKMPEG2_INSTALL_NO_DEVELOPMENT)
    SET(VTKMPEG2_INSTALL_NO_LIBRARIES 1)
  ENDIF(VTKMPEG2_INSTALL_NO_RUNTIME AND VTKMPEG2_INSTALL_NO_DEVELOPMENT)
ELSE(BUILD_SHARED_LIBS)
  IF(VTKMPEG2_INSTALL_NO_DEVELOPMENT)
    SET(VTKMPEG2_INSTALL_NO_LIBRARIES 1)
  ENDIF(VTKMPEG2_INSTALL_NO_DEVELOPMENT)
ENDIF(BUILD_SHARED_LIBS)

ADD_LIBRARY(vtkMPEG2Encode ${srcs})
TARGET_LINK_LIBRARIES(vtkMPEG2Encode vtkImaging)
IF(NOT VTKMPEG2_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS(${VTKMPEG2_INSTALL_LIB_DIR}
                  RUNTIME_DIRECTORY ${VTKMPEG2_INSTALL_BIN_DIR} vtkMPEG2Encode)
ENDIF(NOT VTKMPEG2_INSTALL_NO_LIBRARIES)
