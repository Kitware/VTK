PROJECT(VTKEXPAT)
INCLUDE_REGULAR_EXPRESSION("^(expat|xml|ascii|utf|name|iascii|latin).*$")

SET(expat_SRCS
  xmlparse.c
  xmltok.c
  xmlrole.c
)

INCLUDE_DIRECTORIES(${VTKEXPAT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${VTKEXPAT_BINARY_DIR})

IF(WIN32)
  IF(NOT BUILD_SHARED_LIBS)
    SET (VTK_EXPAT_STATIC 1)
  ENDIF(NOT BUILD_SHARED_LIBS)
ENDIF(WIN32)

CONFIGURE_FILE(${VTKEXPAT_SOURCE_DIR}/expatConfig.h.in
               ${VTKEXPAT_BINARY_DIR}/expatConfig.h)
CONFIGURE_FILE(${VTKEXPAT_SOURCE_DIR}/expatDllConfig.h.in
               ${VTKEXPAT_BINARY_DIR}/expatDllConfig.h)
CONFIGURE_FILE(${VTKEXPAT_SOURCE_DIR}/.NoDartCoverage
               ${VTKEXPAT_BINARY_DIR}/.NoDartCoverage)

ADD_LIBRARY(vtkexpat ${expat_SRCS})

INSTALL_TARGETS(/lib/vtk vtkexpat)
INSTALL_FILES(/include/vtk .h expat expatDllConfig)
