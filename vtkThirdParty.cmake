#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_OPTION upper lower)
  OPTION(VTK_USE_SYSTEM_${upper} "Use the system's ${lower} library." OFF)
  MARK_AS_ADVANCED(VTK_USE_SYSTEM_${upper})
  IF(VTK_USE_SYSTEM_${upper})
    INCLUDE(${CMAKE_ROOT}/Modules/Find${upper}.cmake)
    IF(${upper}_FOUND)
      SET(VTK_${upper}_LIBRARIES ${${upper}_LIBRARIES})
    ELSE(${upper}_FOUND)
      MESSAGE(SEND_ERROR "VTK_USE_SYSTEM_${upper} is ON, but ${upper}_LIBRARY is NOTFOUND.")
    ENDIF(${upper}_FOUND)
  ELSE(VTK_USE_SYSTEM_${upper})
    SET(VTK_${upper}_LIBRARIES vtk${lower})
  ENDIF(VTK_USE_SYSTEM_${upper})
ENDMACRO(VTK_THIRD_PARTY_OPTION)

#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_INCLUDE upper lower)
  IF(VTK_USE_SYSTEM_${upper})
    IF(${upper}_INCLUDE_DIR)
      SET(VTK_INCLUDE_DIRS_SYSTEM ${VTK_INCLUDE_DIRS_SYSTEM} ${${upper}_INCLUDE_DIR})
    ENDIF(${upper}_INCLUDE_DIR)
  ELSE(VTK_USE_SYSTEM_${upper})
    SET(VTK_INCLUDE_DIRS_SOURCE_TREE ${VTK_INCLUDE_DIRS_SOURCE_TREE}
      ${VTK_BINARY_DIR}/Utilities/${lower}
      ${VTK_SOURCE_DIR}/Utilities/${lower}
    )
  ENDIF(VTK_USE_SYSTEM_${upper})
ENDMACRO(VTK_THIRD_PARTY_INCLUDE)

#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_SUBDIR upper lower)
  IF(NOT VTK_USE_SYSTEM_${upper})
    SUBDIRS(${lower})
  ENDIF(NOT VTK_USE_SYSTEM_${upper})
ENDMACRO(VTK_THIRD_PARTY_SUBDIR)
