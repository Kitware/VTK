PROJECT(VTKPNG)
INCLUDE_REGULAR_EXPRESSION("^png.*$")

INCLUDE_DIRECTORIES(${VTKPNG_SOURCE_DIR})
INCLUDE_DIRECTORIES(${VTKPNG_BINARY_DIR})

# source files for png
SET(PNG_SRCS
pngget.c    pngrio.c    pngwrite.c
png.c       pngmem.c    pngrtran.c  pngtrans.c  pngwtran.c
pngerror.c  pngpread.c  pngrutil.c  pngvcrd.c   pngwutil.c
pnggccrd.c  pngread.c   pngset.c   pngwio.c
)

IF(WIN32)
  IF(BUILD_SHARED_LIBS)
    ADD_DEFINITIONS(-DPNG_BUILD_DLL)
    SET(PNG_NO_MODULEDEF 1)
  ELSE(BUILD_SHARED_LIBS)
    SET(PNG_STATIC 1)
  ENDIF(BUILD_SHARED_LIBS)
ENDIF(WIN32)

CONFIGURE_FILE(${VTKPNG_SOURCE_DIR}/.NoDartCoverage
               ${VTKPNG_BINARY_DIR}/.NoDartCoverage)
CONFIGURE_FILE(${VTKPNG_SOURCE_DIR}/pngDllConfig.h.in
               ${VTKPNG_BINARY_DIR}/pngDllConfig.h)

ADD_LIBRARY(vtkpng ${PNG_SRCS})
TARGET_LINK_LIBRARIES(vtkpng ${VTK_ZLIB_LIBRARIES})

INSTALL_TARGETS(/lib/vtk vtkpng)
INSTALL_FILES(/include/vtk .h png pngconf pngDllConfig)
