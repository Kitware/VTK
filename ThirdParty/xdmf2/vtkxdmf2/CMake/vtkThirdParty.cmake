#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_OPTION upper lower)
  OPTION(VTK_USE_SYSTEM_${upper} "Use the system's ${lower} library." OFF)
  MARK_AS_ADVANCED(VTK_USE_SYSTEM_${upper})
  IF(VTK_USE_SYSTEM_${upper})
    IF(EXISTS ${CMAKE_ROOT}/Modules/Find${upper}.cmake)
      INCLUDE(${CMAKE_ROOT}/Modules/Find${upper}.cmake)
    ELSE()
      INCLUDE(${VTK_CMAKE_DIR}/Find${upper}.cmake)
    ENDIF()
    MARK_AS_ADVANCED(${upper}_INCLUDE_DIR ${upper}_LIBRARY)
    IF(${upper}_FOUND)
      SET(VTK_${upper}_LIBRARIES ${${upper}_LIBRARIES})
      IF("${upper}" MATCHES "^PNG$")
        SET(PNG_INCLUDE_DIR ${PNG_PNG_INCLUDE_DIR})
        MARK_AS_ADVANCED(PNG_PNG_INCLUDE_DIR)
      ENDIF()
    ELSE()
      MESSAGE(SEND_ERROR "VTK_USE_SYSTEM_${upper} is ON, but ${upper}_LIBRARY is NOTFOUND.")
    ENDIF()
  ELSE()
    SET(VTK_${upper}_LIBRARIES vtk${lower})
  ENDIF()
ENDMACRO()

#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_INCLUDE upper lower)
  IF(VTK_USE_SYSTEM_${upper})
    IF(${upper}_INCLUDE_DIR)
      SET(VTK_INCLUDE_DIRS_SYSTEM ${VTK_INCLUDE_DIRS_SYSTEM} ${${upper}_INCLUDE_DIR})
    ENDIF()
  ELSE()
    SET(VTK_INCLUDE_DIRS_SOURCE_TREE ${VTK_INCLUDE_DIRS_SOURCE_TREE}
      ${VTK_BINARY_DIR}/Utilities/${lower}
      ${VTK_SOURCE_DIR}/Utilities/${lower}
    )
  ENDIF()
ENDMACRO()

MACRO(VTK_THIRD_PARTY_INCLUDE2 upper)
  IF(VTK_USE_SYSTEM_${upper})
    IF(${upper}_INCLUDE_DIR)
      SET(VTK_INCLUDE_DIRS_SYSTEM ${VTK_INCLUDE_DIRS_SYSTEM} ${${upper}_INCLUDE_DIR})
    ENDIF()
  ENDIF()
ENDMACRO()

#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_SUBDIR upper lower)
  IF(NOT VTK_USE_SYSTEM_${upper})
    SUBDIRS(${lower})
  ENDIF()
ENDMACRO()

#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_WARNING_SUPPRESS upper lang)
  IF(NOT ${upper}_WARNINGS_ALLOW)
    # Visual Studio generators of CMake use /W0 to suppress warnings. 
    # MSVC complains if another warning level is given, so remove it.
    IF(MSVC)
      SET(${upper}_WARNINGS_BLOCKED 1)
      STRING(REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " "
        CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS}")
      SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} /W0")
    ENDIF()

    # Borland uses -w- to suppress warnings.
    IF(BORLAND)
      SET(${upper}_WARNINGS_BLOCKED 1)
      SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w-")
    ENDIF()

    # Most compilers use -w to suppress warnings.
    IF(NOT ${upper}_WARNINGS_BLOCKED)
      SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
    ENDIF()
  ENDIF()
ENDMACRO()
