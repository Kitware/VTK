#-----------------------------------------------------------------------------
MACRO(VTK_EXPORT_KIT kit ukit sources)
  SET(KIT_CLASS_LIST)
  SET(KIT_ABSTRACT_LIST)
  SET(KIT_EXCLUDE_LIST)
  FOREACH(src ${sources})
    STRING(REGEX REPLACE "\\.cxx$" "" CLASS "${src}")
    SET(KIT_CLASS_LIST "${KIT_CLASS_LIST}\n  \"${CLASS}\"")
    GET_SOURCE_FILE_PROPERTY(IS_ABSTRACT ${src} ABSTRACT)
    IF(IS_ABSTRACT MATCHES "^1$")
      SET(KIT_ABSTRACT_LIST "${KIT_ABSTRACT_LIST}\n  \"${CLASS}\"")
    ENDIF(IS_ABSTRACT MATCHES "^1$")
    GET_SOURCE_FILE_PROPERTY(IS_EXCLUDE ${src} WRAP_EXCLUDE)
    IF(IS_EXCLUDE MATCHES "^1$")
      SET(KIT_EXCLUDE_LIST "${KIT_EXCLUDE_LIST}\n  \"${CLASS}\"")
    ENDIF(IS_EXCLUDE MATCHES "^1$")
  ENDFOREACH(src)
  SET(VTK_EXPORT_KIT ${kit})
  SET(VTK_EXPORT_UKIT ${ukit})
  SET(VTK_EXPORT_KIT_DOLLAR "$")
  SET(VTK_EXPORT_HEADER_DIR ${VTK_EXPORT_KIT_DOLLAR}{VTK_INSTALL_PREFIX}${VTK_INSTALL_INCLUDE_DIR})
  CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkKit.cmake.in
                 ${VTK_BINARY_DIR}/Utilities/InstallOnly/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
  SET(VTK_EXPORT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

  # This file is also used when converting Tcl tests to python tests.
  CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkKit.cmake.in
                 ${VTK_BINARY_DIR}/Utilities/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES
      ${VTK_BINARY_DIR}/Utilities/InstallOnly/vtk${kit}Kit.cmake
      DESTINATION "${VTK_INSTALL_PACKAGE_DIR_CM24}"
      COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
ENDMACRO(VTK_EXPORT_KIT)
