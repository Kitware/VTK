PROJECT(VTKTIFF)

# library object files common to compression and decompression
SOURCE_FILES(common_SRCS 
tif_aux.c tif_close.c 
tif_codec.c tif_compress.c tif_dir.c tif_dirinfo.c tif_dirread.c 
tif_dirwrite.c tif_dumpmode.c tif_error.c tif_fax3.c tif_flush.c 
tif_getimage.c tif_jpeg.c tif_luv.c tif_lzw.c tif_next.c 
tif_ojpeg.c tif_open.c tif_packbits.c tif_pixarlog.c tif_predict.c 
tif_print.c tif_read.c tif_strip.c tif_swab.c tif_thunder.c tif_tile.c 
tif_version.c tif_warning.c 
tif_write.c tif_zip.c)

ADD_DEFINITIONS( -DJPEG_SUPPORT -DZIP_SUPPORT -DZLIB_DLL -D_HPUX_SOURCE)

ADD_LIBRARY(vtktiff common_SRCS)
SET(WINDOWS 0)
IF (WIN32)
  IF (NOT UNIX)
  SET(WINDOWS 1)
  ENDIF (NOT UNIX)
ENDIF (WIN32)
 
IF(WINDOWS)
  IF (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS( -DTIFFDLL -DZLIB_DLL -DUSE_WINDOWS_MESSAGEBOX )
  ELSE (BUILD_SHARED_LIBS)
    ADD_DEFINITIONS( -DTIFFSTATIC -DUSE_WINDOWS_MESSAGEBOX )
  ENDIF (BUILD_SHARED_LIBS)
  ADD_DEFINITIONS( -DTIF_PLATFORM_CONSOLE )
  SOURCE_FILES(common_SRCS tif_win32.c fax3sm_winnt.c)
  IF (NOT BORLAND)
    SOURCE_FILES(common_SRCS vtktiff.def)
  ENDIF(NOT BORLAND)
ELSE (WINDOWS)
  IF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)
    IF (CMAKE_MINOR_VERSION MATCHES [2-9][0-9]*)
      SET(TIFF_RIGHT_VERSION 1)
    ENDIF (CMAKE_MINOR_VERSION MATCHES [2-9][0-9]*)
  ENDIF (CMAKE_MAJOR_VERSION MATCHES [1-9][0-9]*)

  IF(TIFF_RIGHT_VERSION)
    SOURCE_FILES(common_SRCS tif_unix.c GENERATED tif_fax3sm.c)
    ADD_EXECUTABLE(mkg3states mkg3states)     
    ADD_CUSTOM_COMMAND(
      SOURCE ${EXECUTABLE_OUTPUT_PATH}/mkg3states
      COMMAND ${EXECUTABLE_OUTPUT_PATH}/mkg3states
      ARGS -c const ${VTK_BINARY_DIR}/Utilities/tiff/tif_fax3sm.c 
      TARGET vtktiff 
      OUTPUTS ${VTK_BINARY_DIR}/Utilities/tiff/tif_fax3sm.c)
  ELSE(TIFF_RIGHT_VERSION)
      MESSAGE("The new TIFF reader requires CMake 1.2 or newer. Please get the new version of CMake from www.cmake.org.")
  ENDIF(TIFF_RIGHT_VERSION)
ENDIF (WINDOWS)

TARGET_LINK_LIBRARIES( vtktiff vtkjpeg vtkzlib )

INSTALL_TARGETS(/lib/vtk vtktiff)
