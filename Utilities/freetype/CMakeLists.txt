PROJECT (VTKFREETYPE)

INCLUDE_REGULAR_EXPRESSION(".*")

INCLUDE_DIRECTORIES (
  ${VTKFREETYPE_SOURCE_DIR}/include
  ${VTKFREETYPE_SOURCE_DIR}/src
)

SOURCE_FILES (FREETYPE_SRCS
  src/base/ftinit.c
  src/base/ftbase.c
  src/base/ftglyph.c
  src/base/ftbbox.c
  src/base/ftmm.c
  src/autohint/autohint.c
  src/cache/ftcache.c
  src/sfnt/sfnt.c
  src/cff/cff.c
  src/psnames/psmodule.c
  src/psaux/psaux.c
  src/truetype/truetype.c
  src/type1/type1.c
  src/cid/type1cid.c
  src/winfonts/winfnt.c
  src/raster/raster.c
  src/smooth/smooth.c
  src/pcf/pcf.c
  src/pshinter/pshinter.c
  src/bdf/bdf.c
  src/pfr/pfr.c
  src/type42/type42.c
)

IF (BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DVTKFREETYPE_DLL)
ELSE (BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DVTKFREETYPE_STATIC)
ENDIF (BUILD_SHARED_LIBS)

IF (WIN32)
  SOURCE_FILES (FREETYPE_SRCS builds/win32/ftdebug.c)
  INCLUDE_DIRECTORIES (
    BEFORE
    ${VTKFREETYPE_SOURCE_DIR}/builds/win32
  )
ELSE (WIN32)
  SOURCE_FILES (FREETYPE_SRCS src/base/ftdebug.c)
ENDIF (WIN32)

IF (UNIX)
  # IF(CMAKE_SYSTEM MATCHES "HP-UX.*")
    ADD_DEFINITIONS(-D_HPUX_SOURCE)
  # ENDIF(CMAKE_SYSTEM MATCHES "HP-UX.*")
  SOURCE_FILES (FREETYPE_SRCS builds/unix/ftsystem.c)
  CONFIGURE_FILE (${VTKFREETYPE_SOURCE_DIR}/builds/unix/ftconfig.h.in
                  ${VTKFREETYPE_BINARY_DIR}/builds/unix/ftconfig.h)
  INCLUDE_DIRECTORIES (
    BEFORE
    ${VTKFREETYPE_BINARY_DIR}/builds/unix
  )
ELSE (UNIX)
  SOURCE_FILES (FREETYPE_SRCS src/base/ftsystem.c)
ENDIF (UNIX)

IF (APPLE)
  ADD_DEFINITIONS(-DTARGET_API_MAC_CARBON)
  SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated -fpascal-strings")
  SET (CMAKE_SHLIB_LINK_FLAGS "${CMAKE_SHLIB_LINK_FLAGS} -multiply_defined suppress -framework Carbon")
  SOURCE_FILES (FREETYPE_SRCS src/base/ftmac.c)
ENDIF (APPLE)

ADD_LIBRARY (vtkfreetype FREETYPE_SRCS)

CONFIGURE_FILE (${VTKFREETYPE_SOURCE_DIR}/.NoDartCoverage
                ${VTKFREETYPE_BINARY_DIR}/.NoDartCoverage)

INSTALL_TARGETS (/lib/vtk vtkfreetype)
