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
  SET(VTK_EXPORT_HEADER_DIR ${CMAKE_INSTALL_PREFIX}/include/vtk)
  CONFIGURE_FILE(${VTK_SOURCE_DIR}/CMake/vtkKit.cmake.in
                 ${VTK_BINARY_DIR}/Utilities/InstallOnly/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
  SET(VTK_EXPORT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  CONFIGURE_FILE(${VTK_SOURCE_DIR}/CMake/vtkKit.cmake.in
                 ${VTK_BINARY_DIR}/Utilities/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
  INSTALL_FILES(/lib/vtk FILES
    ${VTK_BINARY_DIR}/Utilities/InstallOnly/vtk${kit}Kit.cmake
    )
ENDMACRO(VTK_EXPORT_KIT)
