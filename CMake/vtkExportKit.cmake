#-----------------------------------------------------------------------------
MACRO(VTK_EXPORT_KIT kit ukit sources)
  SET(KIT_CLASS_LIST)
  SET(KIT_ABSTRACT_LIST)
  SET(KIT_EXCLUDE_LIST)
  FOREACH(src ${sources})
    STRING(REGEX REPLACE "\\.cxx$" "" CLASS "${src}")
    SET(KIT_CLASS_LIST "${KIT_CLASS_LIST}\n  ${CLASS}")
    GET_SOURCE_FILE_PROPERTY(IS_ABSTRACT ${src} ABSTRACT)
    IF(IS_ABSTRACT MATCHES "^1$")
      SET(KIT_ABSTRACT_LIST "${KIT_ABSTRACT_LIST}\n  ${CLASS}")
    ENDIF(IS_ABSTRACT MATCHES "^1$")
    GET_SOURCE_FILE_PROPERTY(IS_EXCLUDE ${src} WRAP_EXCLUDE)
    IF(IS_EXCLUDE MATCHES "^1$")
      SET(KIT_EXCLUDE_LIST "${KIT_EXCLUDE_LIST}\n  ${CLASS}")
    ENDIF(IS_EXCLUDE MATCHES "^1$")
  ENDFOREACH(src)
  STRING(ASCII 35 POUND)
  STRING(ASCII 36 DOLLAR)
  STRING(ASCII 64 AT)
  WRITE_FILE(${CMAKE_CURRENT_BINARY_DIR}/vtk${kit}Kit.cmake.in.tmp
    "${POUND} Directory containing class headers.\n"
    "SET(VTK_${ukit}_HEADER_DIR \"${AT}VTK_${ukit}_HEADER_DIR${AT}\")\n"
    "\n"
    "${POUND} Classes in vtk${kit}.\n"
    "SET(VTK_${ukit}_CLASSES${KIT_CLASS_LIST}\n  )\n"
    "\n"
    "${POUND} Abstract classes in vtk${kit}.\n"
    "SET(VTK_${ukit}_CLASSES_ABSTRACT${KIT_ABSTRACT_LIST}\n  )\n"
    "\n"
    "${POUND} Wrap-exclude classes in vtk${kit}.\n"
    "SET(VTK_${ukit}_CLASSES_WRAP_EXCLUDE${KIT_EXCLUDE_LIST}\n  )\n"
    "\n"
    "${POUND} Set convenient variables to test each class.\n"
    "FOREACH(class ${DOLLAR}{VTK_${ukit}_CLASSES})\n"
    "  SET(VTK_CLASS_EXISTS_${DOLLAR}{class} 1)\n"
    "ENDFOREACH(class)\n"
    "FOREACH(class ${DOLLAR}{VTK_${ukit}_CLASSES_ABSTRACT})\n"
    "  SET(VTK_CLASS_ABSTRACT_${DOLLAR}{class} 1)\n"
    "ENDFOREACH(class)\n"
    "FOREACH(class ${DOLLAR}{VTK_${ukit}_CLASSES_WRAP_EXCLUDE})\n"
    "  SET(VTK_CLASS_WRAP_EXCLUDE_${DOLLAR}{class} 1)\n"
    "ENDFOREACH(class)\n"
    )
  EXEC_PROGRAM(${CMAKE_COMMAND} ARGS -E copy_if_different 
    ${CMAKE_CURRENT_BINARY_DIR}/vtk${kit}Kit.cmake.in.tmp
    ${CMAKE_CURRENT_BINARY_DIR}/vtk${kit}Kit.cmake.in OUTPUT_VARIABLE OUT)
    
  SET(VTK_${ukit}_HEADER_DIR ${CMAKE_INSTALL_PREFIX}/include/vtk)
  CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/vtk${kit}Kit.cmake.in
                 ${VTK_BINARY_DIR}/Utilities/InstallOnly/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
  SET(VTK_${ukit}_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/vtk${kit}Kit.cmake.in
                 ${VTK_BINARY_DIR}/Utilities/vtk${kit}Kit.cmake
                 @ONLY IMMEDIATE)
  INSTALL_FILES(/lib/vtk FILES
    ${VTK_BINARY_DIR}/Utilities/InstallOnly/vtk${kit}Kit.cmake
    )
ENDMACRO(VTK_EXPORT_KIT)
