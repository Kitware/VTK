PROJECT(VTKTIFF)
INCLUDE_REGULAR_EXPRESSION("^(tif|fax|port|uv|vtk|tconf|t4|mk|z).*$")

INCLUDE_DIRECTORIES(BEFORE ${VTKTIFF_SOURCE_DIR})
INCLUDE_DIRECTORIES(BEFORE ${VTKTIFF_BINARY_DIR})

# library object files common to compression and decompression
SET(common_SRCS
tif_aux.c tif_close.c
tif_codec.c tif_compress.c tif_dir.c tif_dirinfo.c tif_dirread.c
tif_dirwrite.c tif_dumpmode.c tif_error.c tif_fax3.c tif_flush.c
tif_getimage.c tif_jpeg.c tif_luv.c tif_lzw.c tif_next.c
tif_ojpeg.c tif_open.c tif_packbits.c tif_pixarlog.c tif_predict.c
tif_print.c tif_read.c tif_strip.c tif_swab.c tif_thunder.c tif_tile.c
tif_version.c tif_warning.c
tif_write.c tif_zip.c tif_color.c tif_extension.c )


ADD_DEFINITIONS( -DJPEG_SUPPORT -DZIP_SUPPORT -D_HPUX_SOURCE)

IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    SET(TIFFDLL 1)
  ELSE(BUILD_SHARED_LIBS)
    SET(TIFFSTATIC 1)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(WIN32)

IF (BORLAND)
  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-8004 -w-8057")
ENDIF (BORLAND)

IF(NOT UNIX)
  ADD_DEFINITIONS( -DTIF_PLATFORM_CONSOLE )
  SET(common_SRCS ${common_SRCS} tif_win32.c fax3sm_winnt.c)
  IF(BUILD_SHARED_LIBS)
    IF (NOT BORLAND)
      IF(NOT MINGW)
        SET(common_SRCS ${common_SRCS} vtktiff.def)
      ENDIF(NOT MINGW)
    ENDIF(NOT BORLAND)
  ENDIF(BUILD_SHARED_LIBS)
ELSE (NOT UNIX)
  SET(common_SRCS ${common_SRCS} tif_unix.c tif_fax3sm.c)
  IF(NOT CMAKE_CROSSCOMPILING)
    ADD_EXECUTABLE(vtkmkg3states mkg3states.c)
    vtk_compile_tools_target(vtkmkg3states)

    ADD_CUSTOM_COMMAND(
      OUTPUT ${VTKTIFF_BINARY_DIR}/tif_fax3sm.c
      COMMAND vtkmkg3states
      ARGS -c const ${VTKTIFF_BINARY_DIR}/tif_fax3sm.c
      DEPENDS vtkmkg3states)
  ENDIF(NOT CMAKE_CROSSCOMPILING)
ENDIF (NOT UNIX)

INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFiles.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)
INCLUDE(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)

# Check if header file exists and add it to the list.
CHECK_INCLUDE_FILES("assert.h" HAVE_ASSERT_H)
CHECK_INCLUDE_FILES("dlfcn.h" HAVE_DLFCN_H)
CHECK_INCLUDE_FILES("fcntl.h" HAVE_FCNTL_H)
CHECK_INCLUDE_FILES("inttypes.h" HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES("limits.h" HAVE_LIMITS_H)
CHECK_INCLUDE_FILES("malloc.h" HAVE_MALLOC_H)
CHECK_INCLUDE_FILES("memory.h" HAVE_MEMORY_H)
CHECK_INCLUDE_FILES("stdint.h" HAVE_STDINT_H)
CHECK_INCLUDE_FILES("stdlib.h" HAVE_STDLIB_H)
CHECK_INCLUDE_FILES("string.h" HAVE_STRING_H)
CHECK_INCLUDE_FILES("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILES("sys/stat.h" HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES("sys/time.h" HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES("sys/types.h" HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES("unistd.h" HAVE_UNISTD_H)
CHECK_INCLUDE_FILES("windows.h" HAVE_WINDOWS_H)
CHECK_INCLUDE_FILES("strings.h" HAVE_STRINGS_H)
CHECK_INCLUDE_FILES("ieeefp.h" HAVE_IEEEFP_H)

CHECK_FUNCTION_EXISTS(floor HAVE_FLOOR)
CHECK_FUNCTION_EXISTS(getopt HAVE_GETOPT)
CHECK_FUNCTION_EXISTS(isascii HAVE_ISASCII)
CHECK_FUNCTION_EXISTS(memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
CHECK_FUNCTION_EXISTS(mmap HAVE_MMAP)
CHECK_FUNCTION_EXISTS(pow HAVE_POW)
CHECK_FUNCTION_EXISTS(sqrt HAVE_SQRT)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
CHECK_FUNCTION_EXISTS(strchr HAVE_STRCHR)
CHECK_FUNCTION_EXISTS(strrchr HAVE_STRRCHR)
CHECK_FUNCTION_EXISTS(strstr HAVE_STRSTR)
CHECK_FUNCTION_EXISTS(strtol HAVE_STRTOL)
CHECK_FUNCTION_EXISTS(strtoul HAVE_STRTOUL)


IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.0)
ELSE("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.0)
  INCLUDE(CheckTypeSize)
ENDIF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 2.0)

CHECK_TYPE_SIZE("int" SIZEOF_INT)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)

CONFIGURE_FILE(${VTKTIFF_SOURCE_DIR}/tiffDllConfig.h.in
               ${VTKTIFF_BINARY_DIR}/tiffDllConfig.h)
CONFIGURE_FILE(${VTKTIFF_SOURCE_DIR}/tif_config.h.in
  ${VTKTIFF_BINARY_DIR}/tif_config.h)

VTK_ADD_LIBRARY(vtktiff ${common_SRCS})
TARGET_LINK_LIBRARIES( vtktiff ${vtkzlib_LIBRARIES} ${vtkjpeg_LIBRARIES} )

# link against the math library
IF(UNIX)
  TARGET_LINK_LIBRARIES(vtktiff -lm)
ENDIF()

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKTIFF_BINARY_DIR}/tiffDllConfig.h
    ${VTKTIFF_SOURCE_DIR}/tiffvers.h
    ${VTKTIFF_SOURCE_DIR}/vtk_tiff_mangle.h
    ${VTKTIFF_SOURCE_DIR}/tiff.h
    ${VTKTIFF_SOURCE_DIR}/tiffio.h
    ${VTKTIFF_SOURCE_DIR}/tconf.h
    ${VTKTIFF_SOURCE_DIR}/tiffconf.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtktiff
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
