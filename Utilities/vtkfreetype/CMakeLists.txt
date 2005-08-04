PROJECT (VTKFREETYPE)

#
# Dependency mask
#
INCLUDE_REGULAR_EXPRESSION (".*")

#
# Include paths
#
INCLUDE_DIRECTORIES (
  ${VTKFREETYPE_SOURCE_DIR}/include
  ${VTKFREETYPE_SOURCE_DIR}/src
  ${VTKFREETYPE_BINARY_DIR}
  ${VTKFREETYPE_BINARY_DIR}/include
)

#
# Source files
#
SET (FREETYPE_SRCS
  src/autohint/autohint.c
  src/base/ftbase.c
  src/base/ftbbox.c
  src/base/ftglyph.c
  src/base/ftinit.c
  src/base/ftmm.c
  src/bdf/bdf.c
  src/cache/ftcache.c
  src/cff/cff.c
  src/cid/type1cid.c
  src/gzip/ftgzip.c
  src/lzw/ftlzw.c
  src/pcf/pcf.c
  src/pfr/pfr.c
  src/psaux/psaux.c
  src/pshinter/pshinter.c
  src/psnames/psmodule.c
  src/raster/raster.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/truetype/truetype.c
  src/type1/type1.c
  src/type42/type42.c
  src/winfonts/winfnt.c
)

#
# Static or shared lib flags
#
IF (BUILD_SHARED_LIBS)
  SET (VTKFREETYPE_DLL 1)
ELSE (BUILD_SHARED_LIBS)
  SET (VTKFREETYPE_STATIC 1)
ENDIF (BUILD_SHARED_LIBS)

#
# Win32 flags and specific drivers
#
IF (WIN32)
  SET (FREETYPE_SRCS ${FREETYPE_SRCS} builds/win32/ftdebug.c)
  INCLUDE_DIRECTORIES (
    BEFORE
    ${VTKFREETYPE_SOURCE_DIR}/builds/win32
  )
  IF (BORLAND)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-8004")
  ENDIF (BORLAND)
ELSE (WIN32)
  SET (FREETYPE_SRCS ${FREETYPE_SRCS} src/base/ftdebug.c)
ENDIF (WIN32)

#
# Unix flags and specific drivers
#
IF (UNIX)
  IF (CMAKE_SYSTEM MATCHES "HP-UX.*")
    SET (VTKFREETYPE_HPUX_SOURCE 1)
  ENDIF (CMAKE_SYSTEM MATCHES "HP-UX.*")
  SET (FREETYPE_SRCS ${FREETYPE_SRCS} builds/unix/ftsystem.c)
  INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)
  CHECK_INCLUDE_FILE_CXX("fcntl.h" HAVE_FCNTL_H)
  CHECK_INCLUDE_FILE_CXX("unistd.h" HAVE_UNISTD_H)
  CONFIGURE_FILE (${VTKFREETYPE_SOURCE_DIR}/builds/unix/ftconfig.h.in
                  ${VTKFREETYPE_BINARY_DIR}/builds/unix/ftconfig.h)
  INCLUDE_DIRECTORIES (
    BEFORE
    ${VTKFREETYPE_BINARY_DIR}/builds/unix
  )
ELSE (UNIX)
  SET (FREETYPE_SRCS ${FREETYPE_SRCS} src/base/ftsystem.c)
ENDIF (UNIX)

#
# Apple flags and specific drivers
#
IF (APPLE)
  SET (VTKFREETYPE_USE_CARBON 0)
#  SET (FREETYPE_SRCS ${FREETYPE_SRCS} src/base/ftmac.c)
ENDIF (APPLE)

#
# Define the library (and install it)
#
ADD_LIBRARY (vtkfreetype ${FREETYPE_SRCS})
IF (APPLE)
  TARGET_LINK_LIBRARIES (vtkfreetype "-framework AppKit")
ENDIF (APPLE)

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(vtkfreetype PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL_TARGETS (${VTK_INSTALL_LIB_DIR} vtkfreetype)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)

#
# Do not cover this lib
#
CONFIGURE_FILE (${VTKFREETYPE_SOURCE_DIR}/.NoDartCoverage
                ${VTKFREETYPE_BINARY_DIR}/.NoDartCoverage)

#
# Create the configuration file
#
ADD_DEFINITIONS (-DVTKFREETYPE)
CONFIGURE_FILE (${VTKFREETYPE_SOURCE_DIR}/include/vtkFreeTypeConfig.h.in
                ${VTKFREETYPE_BINARY_DIR}/include/vtkFreeTypeConfig.h)

