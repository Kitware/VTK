# - Check if the source code provided in the SOURCE argument compiles.
# CHECK_CXX_SOURCE_COMPILES(SOURCE VAR)
# - macro which checks if the source code compiles\
#  SOURCE - source code to try to compile
#  VAR    - variable to store size if the type exists.
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link

MACRO(CHECK_CXX_SOURCE_RUNS SOURCE VAR COMMENT)
  IF(NOT DEFINED "HAVE_${VAR}")
    SET(MACRO_CHECK_FUNCTION_DEFINITIONS
      "-D${VAR} ${CMAKE_REQUIRED_FLAGS}")
    IF(CMAKE_REQUIRED_LIBRARIES)
      SET(CHECK_CXX_SOURCE_COMPILES_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ELSE()
      SET(CHECK_CXX_SOURCE_COMPILES_ADD_LIBRARIES)
    ENDIF()
    IF(CMAKE_REQUIRED_INCLUDES)
      SET(CHECK_CXX_SOURCE_COMPILES_ADD_INCLUDES
        "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
    ELSE()
      SET(CHECK_CXX_SOURCE_COMPILES_ADD_INCLUDES)
    ENDIF()
    SET(CMAKE_EMPTY_INPUT_FILE_CONTENT "${SOURCE}")
    CONFIGURE_FILE("${VTK_SOURCE_DIR}/CMake/CMakeEmptyInputFile.in"
      "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/src.cxx")

    MESSAGE(STATUS "Performing Test ${COMMENT}")
    TRY_RUN(${VAR} HAVE_${VAR}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/src.cxx
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
      "${CHECK_CXX_SOURCE_COMPILES_ADD_LIBRARIES}"
      "${CHECK_CXX_SOURCE_COMPILES_ADD_INCLUDES}"
      OUTPUT_VARIABLE OUTPUT)
    IF(HAVE_${VAR})
      SET(${VAR} 1 CACHE INTERNAL "Test ${COMMENT}")
      MESSAGE(STATUS "Performing Test ${COMMENT} - Success")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Performing C++ SOURCE FILE Test ${COMMENT} succeded with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
    ELSE()
      MESSAGE(STATUS "Performing Test ${COMMENT} - Failed")
      SET(${VAR} "" CACHE INTERNAL "Test ${COMMENT}")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Performing C++ SOURCE FILE Test ${COMMENT} failed with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
    ENDIF()
  ENDIF()
ENDMACRO()


