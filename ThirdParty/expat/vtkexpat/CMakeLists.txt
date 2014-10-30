PROJECT(VTKEXPAT)
INCLUDE_REGULAR_EXPRESSION("^(expat|xml|ascii|utf|name|iascii|latin|vtk).*$")

SET(expat_SRCS
  xmlparse.c
  xmltok.c
  xmlrole.c
)

INCLUDE_DIRECTORIES(${VTKEXPAT_SOURCE_DIR})

IF(WIN32)
  IF(NOT BUILD_SHARED_LIBS)
    SET (VTK_EXPAT_STATIC 1)
  ENDIF()
ENDIF()

CONFIGURE_FILE(${VTKEXPAT_SOURCE_DIR}/expatConfig.h.in
               ${VTKEXPAT_BINARY_DIR}/expatConfig.h)
CONFIGURE_FILE(${VTKEXPAT_SOURCE_DIR}/expatDllConfig.h.in
               ${VTKEXPAT_BINARY_DIR}/expatDllConfig.h)

VTK_ADD_LIBRARY(vtkexpat ${expat_SRCS})

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKEXPAT_BINARY_DIR}/expatDllConfig.h
    ${VTKEXPAT_SOURCE_DIR}/expat.h
    ${VTKEXPAT_SOURCE_DIR}/vtk_expat_mangle.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtkexpat
    COMPONENT Development)
ENDIF()
