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
  src/base/ftbase.c
  src/base/ftbbox.c
  src/base/ftbitmap.c
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
  CONFIGURE_FILE (builds/unix/ftconfig.in
                  include/freetype/config/ftconfig.h)
  SET(freetype_config_headers_exclude
    ${VTKFREETYPE_SOURCE_DIR}/include/freetype/config/ftconfig.h)
  SET(freetype_config_headers_include
    ${VTKFREETYPE_BINARY_DIR}/include/freetype/config/ftconfig.h)
ELSE (UNIX)
  SET (FREETYPE_SRCS ${FREETYPE_SRCS} src/base/ftsystem.c)
ENDIF (UNIX)

#
# freetype can be optionally built with support for resource
# fork-based Mac fonts. That support requires linking against
# Mac OS X frameworks, and so is disabled if using X11.
#
IF (APPLE AND NOT VTK_USE_X)
  SET (VTKFREETYPE_MAC_FONT_SUPPORT 1)
ENDIF (APPLE AND NOT VTK_USE_X)

#
# Define the library (and install it)
#
VTK_ADD_LIBRARY (vtkfreetype ${FREETYPE_SRCS})

#
# Link to vtkzlib
#
TARGET_LINK_LIBRARIES(vtkfreetype ${vtkzlib_LIBRARIES})

#
# If we want freetype's Mac font support we need to link against
# a couple of Mac OS X frameworks.
#
IF (APPLE AND VTKFREETYPE_MAC_FONT_SUPPORT)
    TARGET_LINK_LIBRARIES (vtkfreetype "-framework ApplicationServices -framework CoreServices")
ENDIF (APPLE AND VTKFREETYPE_MAC_FONT_SUPPORT)

#
# Create the configuration file
#
ADD_DEFINITIONS (-DFT2_BUILD_LIBRARY)
CONFIGURE_FILE (${VTKFREETYPE_SOURCE_DIR}/include/vtkFreeTypeConfig.h.in
                ${VTKFREETYPE_BINARY_DIR}/include/vtkFreeTypeConfig.h)


#
# Install headers
#
#
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKFREETYPE_SOURCE_DIR}/include/ft2build.h
    ${VTKFREETYPE_SOURCE_DIR}/include/vtk_freetype_mangle.h
    ${VTKFREETYPE_SOURCE_DIR}/include/vtk_ftmodule.h
    ${VTKFREETYPE_BINARY_DIR}/include/vtkFreeTypeConfig.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkfreetype/include
    COMPONENT Development)

  file(GLOB freetype_headers 
    "${VTKFREETYPE_SOURCE_DIR}/include/freetype/*.h")
  INSTALL(FILES ${freetype_headers}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkfreetype/include/freetype
    COMPONENT Development)

  file(GLOB freetype_config_headers 
    "${VTKFREETYPE_SOURCE_DIR}/include/freetype/config/*.h")
  if(freetype_config_headers_exclude)
    list(REMOVE_ITEM freetype_config_headers ${freetype_config_headers_exclude})
  endif()
  INSTALL(FILES ${freetype_config_headers}
                ${freetype_config_headers_include}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkfreetype/include/freetype/config
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
