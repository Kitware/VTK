#
# Check if the system supports 64 bit streams
#
# CHECK_FOR_64BIT_STREAMS - macro which checks the existence of 64 bit streams
# VARIABLE - variable to return result
#

MACRO(CHECK_FOR_64BIT_STREAMS VARIABLE)
  IF(NOT DEFINED "${VARIABLE}")
    SET(MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS ${CMAKE_REQUIRED_FLAGS})
    IF(CMAKE_NO_ANSI_STREAM_HEADERS)
      SET(MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS
        "${MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS} -DNO_ANSI")
    ENDIF()
    IF(SIZEOF_LONG_LONG)
      SET(MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS 
        "${MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS} -DSIZEOF_LONG_LONG")
    ENDIF()
    IF(SIZEOF___INT64)
      SET(MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS 
        "${MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS} -DSIZEOF___INT64")
    ENDIF()
    MESSAGE(STATUS "Check if system supports 64 bit streams")
    TRY_COMPILE(${VARIABLE}
               ${CMAKE_BINARY_DIR}
               ${xdmf2_SOURCE_DIR}/CMake/CheckFor64BitStreams.cxx
               CMAKE_FLAGS 
               -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FOR_64BIT_STREAMS_FLAGS}
               OUTPUT_VARIABLE OUTPUT)
    IF(${VARIABLE})
      MESSAGE(STATUS "Check if system supports 64 bit streams - yes")
      SET(${VARIABLE} 1 CACHE INTERNAL "Whether streams support 64-bit types")
    ELSE()
      MESSAGE(STATUS "Check if system supports 64 bit streams - no")
      SET(${VARIABLE} "" CACHE INTERNAL "Whether streams support 64-bit types")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining if the system supports 64 bit streams "
        "failed with the following output:\n"
        "${OUTPUT}\n")
    ENDIF()
  ENDIF()
ENDMACRO()
