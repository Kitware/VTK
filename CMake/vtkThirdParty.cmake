#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_OPTION upper lower)
  OPTION(VTK_USE_SYSTEM_${upper} "Use the system's ${lower} library." OFF)
  MARK_AS_ADVANCED(VTK_USE_SYSTEM_${upper})
  IF(VTK_USE_SYSTEM_${upper})
    IF(EXISTS ${CMAKE_ROOT}/Modules/Find${upper}.cmake)
      INCLUDE(${CMAKE_ROOT}/Modules/Find${upper}.cmake)
    ELSE(EXISTS ${CMAKE_ROOT}/Modules/Find${upper}.cmake)
      INCLUDE(${VTK_CMAKE_DIR}/Find${upper}.cmake)
    ENDIF(EXISTS ${CMAKE_ROOT}/Modules/Find${upper}.cmake)
    MARK_AS_ADVANCED(${upper}_INCLUDE_DIR ${upper}_LIBRARY)
    IF(${upper}_FOUND)
      SET(VTK_${upper}_LIBRARIES ${${upper}_LIBRARIES})
      IF("${upper}" MATCHES "^PNG$")
        SET(PNG_INCLUDE_DIR ${PNG_PNG_INCLUDE_DIR})
        MARK_AS_ADVANCED(PNG_PNG_INCLUDE_DIR)
      ENDIF("${upper}" MATCHES "^PNG$")
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
      SET(VTK_${upper}_INCLUDE_DIR ${${upper}_INCLUDE_DIR})
    ENDIF(${upper}_INCLUDE_DIR)
  ELSE(VTK_USE_SYSTEM_${upper})
    SET(VTK_INCLUDE_DIRS_SOURCE_TREE ${VTK_INCLUDE_DIRS_SOURCE_TREE}
      ${VTK_BINARY_DIR}/Utilities/${lower}
      ${VTK_SOURCE_DIR}/Utilities/${lower}
    )
    SET(VTK_${upper}_INCLUDE_DIR
      ${VTK_BINARY_DIR}/Utilities/${lower}
      ${VTK_SOURCE_DIR}/Utilities/${lower}
    )
  ENDIF(VTK_USE_SYSTEM_${upper})
ENDMACRO(VTK_THIRD_PARTY_INCLUDE)

MACRO(VTK_THIRD_PARTY_INCLUDE2 upper)
  IF(VTK_USE_SYSTEM_${upper})
    IF(${upper}_INCLUDE_DIR)
      SET(VTK_INCLUDE_DIRS_SYSTEM ${VTK_INCLUDE_DIRS_SYSTEM} ${${upper}_INCLUDE_DIR})
    ENDIF(${upper}_INCLUDE_DIR)
  ENDIF(VTK_USE_SYSTEM_${upper})
ENDMACRO(VTK_THIRD_PARTY_INCLUDE2)

#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_SUBDIR upper lower)
  IF(NOT VTK_USE_SYSTEM_${upper})
    # we don't want to build third party tests.
    SET(__vtk_build_testing ${BUILD_TESTING})
    SET(BUILD_TESTING OFF)
    ADD_SUBDIRECTORY(${lower})
    # restore BUILD_TESTING
    SET (BUILD_TESTING ${__vtk_build_testing})
  ENDIF(NOT VTK_USE_SYSTEM_${upper})
ENDMACRO(VTK_THIRD_PARTY_SUBDIR)

#-----------------------------------------------------------------------------
MACRO(VTK_THIRD_PARTY_WARNING_SUPPRESS upper lang)
  IF(NOT ${upper}_WARNINGS_ALLOW)
    # MSVC uses /w to suppress warnings.  It also complains if another
    # warning level is given, so remove it.
    IF(MSVC)
      SET(${upper}_WARNINGS_BLOCKED 1)
      STRING(REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " "
        CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS}")
      SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} /w")
    ENDIF(MSVC)

    # Borland uses -w- to suppress warnings.
    IF(BORLAND)
      SET(${upper}_WARNINGS_BLOCKED 1)
      SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w-")
    ENDIF(BORLAND)

    # Most compilers use -w to suppress warnings.
    IF(NOT ${upper}_WARNINGS_BLOCKED)
      SET(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
    ENDIF(NOT ${upper}_WARNINGS_BLOCKED)
  ENDIF(NOT ${upper}_WARNINGS_ALLOW)
ENDMACRO(VTK_THIRD_PARTY_WARNING_SUPPRESS)
