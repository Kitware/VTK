PROJECT(VTKPNG)
INCLUDE_REGULAR_EXPRESSION("^(png|vtk|z).*$")

INCLUDE_DIRECTORIES(${VTKPNG_SOURCE_DIR})

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

CONFIGURE_FILE(${VTKPNG_SOURCE_DIR}/pngDllConfig.h.in
               ${VTKPNG_BINARY_DIR}/pngDllConfig.h)

VTK_ADD_LIBRARY(vtkpng ${PNG_SRCS})

TARGET_LINK_LIBRARIES(vtkpng ${vtkzlib_LIBRARIES})

# link against the math library
IF(UNIX)
  TARGET_LINK_LIBRARIES(vtkpng -lm)
ENDIF()

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKPNG_BINARY_DIR}/pngDllConfig.h
    ${VTKPNG_SOURCE_DIR}/png.h
    ${VTKPNG_SOURCE_DIR}/pngconf.h
    ${VTKPNG_SOURCE_DIR}/vtk_png_mangle.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtkpng
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
