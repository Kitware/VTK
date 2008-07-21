#-----------------------------------------------------------------------------
MACRO(VTK_EXPORT_KIT2 kit ukit dest_dir sources)
  SET(VTK_EXPORT_KIT ${kit})
  SET(VTK_EXPORT_UKIT ${ukit})
  SET(VTK_EXPORT_KIT_DOLLAR "$")

  # Export the list of classes from the install tree.
  SET(VTK_EXPORT_HEADER_DIR ${VTK_EXPORT_KIT_DOLLAR}{VTK_INSTALL_PREFIX}${VTK_INSTALL_INCLUDE_DIR})
  SET(KIT_CLASS_LIST)
  SET(KIT_ABSTRACT_LIST)
  SET(KIT_EXCLUDE_LIST)
  FOREACH(src ${sources})
    # Get the class name from the full file name.  All class headers
    # will be installed to the same directory for this kit in the
    # install tree.
    GET_FILENAME_COMPONENT(CLASS "${src}" NAME_WE)
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
  CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkKit.cmake.in
                 ${dest_dir}/InstallOnly/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
  IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
    INSTALL(FILES
      ${dest_dir}/InstallOnly/vtk${kit}Kit.cmake
      DESTINATION "${VTK_INSTALL_PACKAGE_DIR_CM24}"
      COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

  # Export the list of classes from the build tree.
  # This file is also used when converting Tcl tests to python tests.
  SET(VTK_EXPORT_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  SET(KIT_CLASS_LIST)
  SET(KIT_ABSTRACT_LIST)
  SET(KIT_EXCLUDE_LIST)
  FOREACH(src ${sources})
    # Get the class name with the directory if any.  Some class
    # headers may be in the build tree while others are in the source
    # tree.
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
  CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkKit.cmake.in
                 ${dest_dir}/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
ENDMACRO(VTK_EXPORT_KIT2)

#-----------------------------------------------------------------------------
MACRO(VTK_EXPORT_KIT kit ukit sources)
  VTK_EXPORT_KIT2(${kit} ${ukit} ${VTK_BINARY_DIR}/Utilities "${sources}")
ENDMACRO(VTK_EXPORT_KIT)
