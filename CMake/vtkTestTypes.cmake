SET(VTK_SIZEOF_CHAR   ${CMAKE_SIZEOF_CHAR})
SET(VTK_SIZEOF_DOUBLE ${CMAKE_SIZEOF_DOUBLE})
SET(VTK_SIZEOF_FLOAT  ${CMAKE_SIZEOF_FLOAT})
SET(VTK_SIZEOF_INT    ${CMAKE_SIZEOF_INT})
SET(VTK_SIZEOF_LONG   ${CMAKE_SIZEOF_LONG})
SET(VTK_SIZEOF_SHORT  ${CMAKE_SIZEOF_SHORT})
CHECK_TYPE_SIZE("long long" VTK_SIZEOF_LONG_LONG)
CHECK_TYPE_SIZE("__int64"   VTK_SIZEOF___INT64)

IF(VTK_SIZEOF___INT64)
  # In CMake 2.6 and above the type __int64 may have been found only
  # due to inclusion of a system header.  Further try-compiles using
  # the type should include the header too.
  SET(_HAVE_DEFS)
  FOREACH(def HAVE_SYS_TYPES_H HAVE_STDINT_H HAVE_STDDEF_H)
    IF(${def})
      LIST(APPEND _HAVE_DEFS -D${def})
    ENDIF(${def})
  ENDFOREACH(def)

  IF("VTK_TYPE_SAME_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_AND___INT64$")
    MESSAGE(STATUS "Checking whether long and __int64 are the same type")
    TRY_COMPILE(VTK_TYPE_SAME_LONG_AND___INT64
      ${VTK_BINARY_DIR}/CMakeTmp
      ${VTK_CMAKE_DIR}/vtkTestCompareTypes.cxx
      COMPILE_DEFINITIONS
      -DVTK_TEST_COMPARE_TYPE_1=long
      -DVTK_TEST_COMPARE_TYPE_2=__int64
      ${_HAVE_DEFS}
      OUTPUT_VARIABLE OUTPUT)
    IF(VTK_TYPE_SAME_LONG_AND___INT64)
      MESSAGE(STATUS "Checking whether long and __int64 are the same type -- yes")
      SET(VTK_TYPE_SAME_LONG_AND___INT64 1 CACHE INTERNAL "Whether long and __int64 are the same type")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Determining whether long and __int64 are the same type "
        "passed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ELSE(VTK_TYPE_SAME_LONG_AND___INT64)
      MESSAGE(STATUS "Checking whether long and __int64 are the same type -- no")
      SET(VTK_TYPE_SAME_LONG_AND___INT64 0 CACHE INTERNAL "Whether long and __int64 are the same type")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining whether long and __int64 are the same type "
        "failed with the following output:\n"
        "${OUTPUT}\n" APPEND)
    ENDIF(VTK_TYPE_SAME_LONG_AND___INT64)
  ENDIF("VTK_TYPE_SAME_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_AND___INT64$")
  IF(VTK_SIZEOF_LONG_LONG)
    IF("VTK_TYPE_SAME_LONG_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_LONG_AND___INT64$")
      MESSAGE(STATUS "Checking whether long long and __int64 are the same type")
      TRY_COMPILE(VTK_TYPE_SAME_LONG_LONG_AND___INT64
        ${VTK_BINARY_DIR}/CMakeTmp
        ${VTK_CMAKE_DIR}/vtkTestCompareTypes.cxx
        COMPILE_DEFINITIONS
        -DVTK_TEST_COMPARE_TYPE_1=TYPE_LONG_LONG
        -DVTK_TEST_COMPARE_TYPE_2=__int64
        ${_HAVE_DEFS}
        OUTPUT_VARIABLE OUTPUT)
      IF(VTK_TYPE_SAME_LONG_LONG_AND___INT64)
        MESSAGE(STATUS "Checking whether long long and __int64 are the same type -- yes")
        SET(VTK_TYPE_SAME_LONG_LONG_AND___INT64 1 CACHE INTERNAL "Whether long long and __int64 are the same type")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
          "Determining whether long long and __int64 are the same type "
          "passed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ELSE(VTK_TYPE_SAME_LONG_LONG_AND___INT64)
        MESSAGE(STATUS "Checking whether long long and __int64 are the same type -- no")
        SET(VTK_TYPE_SAME_LONG_LONG_AND___INT64 0 CACHE INTERNAL "Whether long long and __int64 are the same type")
        WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Determining whether long long and __int64 are the same type "
          "failed with the following output:\n"
          "${OUTPUT}\n" APPEND)
      ENDIF(VTK_TYPE_SAME_LONG_LONG_AND___INT64)
    ENDIF("VTK_TYPE_SAME_LONG_LONG_AND___INT64" MATCHES "^VTK_TYPE_SAME_LONG_LONG_AND___INT64$")
  ENDIF(VTK_SIZEOF_LONG_LONG)
  IF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
    IF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
#  VS 6 cannot convert unsigned __int64 to double unless the
# "Visual C++ Processor Pack" is installed.
      IF("VTK_TYPE_CONVERT_UI64_TO_DOUBLE" MATCHES "^VTK_TYPE_CONVERT_UI64_TO_DOUBLE$")
        MESSAGE(STATUS "Checking whether unsigned __int64 can convert to double")
        TRY_COMPILE(VTK_TYPE_CONVERT_UI64_TO_DOUBLE
          ${VTK_BINARY_DIR}/CMakeTmp
          ${VTK_CMAKE_DIR}/vtkTestConvertTypes.cxx
          COMPILE_DEFINITIONS
          -DVTK_TEST_CONVERT_TYPE_FROM=TYPE_UNSIGNED___INT64
          -DVTK_TEST_CONVERT_TYPE_TO=double
          ${_HAVE_DEFS}
          OUTPUT_VARIABLE OUTPUT)
        IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
          MESSAGE(STATUS "Checking whether unsigned __int64 can convert to double -- yes")
          SET(VTK_TYPE_CONVERT_UI64_TO_DOUBLE 1 CACHE INTERNAL "Whether unsigned __int64 can convert to double")
          WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
            "Determining whether unsigned __int64 can convert to double "
            "passed with the following output:\n"
            "${OUTPUT}\n" APPEND)
        ELSE(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
          MESSAGE(STATUS "Checking whether unsigned __int64 can convert to double -- no")
          SET(VTK_TYPE_CONVERT_UI64_TO_DOUBLE 0 CACHE INTERNAL "Whether unsigned __int64 can convert to double")
          WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
            "Determining whether unsigned __int64 can convert to double "
            "failed with the following output:\n"
            "${OUTPUT}\n" APPEND)
        ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
      ENDIF("VTK_TYPE_CONVERT_UI64_TO_DOUBLE" MATCHES "^VTK_TYPE_CONVERT_UI64_TO_DOUBLE$")
    ENDIF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
  ENDIF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
ENDIF(VTK_SIZEOF___INT64)

# Enable the "long long" type if it is available.  It is standard in
# C99 and C++03 but not in earlier standards.
SET(VTK_TYPE_USE_LONG_LONG)
IF(VTK_SIZEOF_LONG_LONG)
  SET(VTK_TYPE_USE_LONG_LONG 1)
ENDIF(VTK_SIZEOF_LONG_LONG)

# Enable the "__int64" type if it is available and unique.  It is not
# standard.
SET(VTK_TYPE_USE___INT64)
IF(VTK_SIZEOF___INT64)
  IF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
    IF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
      SET(VTK_TYPE_USE___INT64 1)
    ENDIF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
  ENDIF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
ENDIF(VTK_SIZEOF___INT64)

IF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")
  MESSAGE(STATUS "Checking support for C++ type bool")
  TRY_COMPILE(VTK_COMPILER_HAS_BOOL
              ${VTK_BINARY_DIR}/CMakeTmp/Bool
              ${VTK_CMAKE_DIR}/vtkTestBoolType.cxx
              OUTPUT_VARIABLE OUTPUT)
  IF(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- yes")
    SET(VTK_COMPILER_HAS_BOOL 1 CACHE INTERNAL "Support for C++ type bool")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if the C++ compiler supports type bool "
      "passed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ELSE(VTK_COMPILER_HAS_BOOL)
    MESSAGE(STATUS "Checking support for C++ type bool -- no")
    SET(VTK_COMPILER_HAS_BOOL 0 CACHE INTERNAL "Support for C++ type bool")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
      "Determining if the C++ compiler supports type bool "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_COMPILER_HAS_BOOL)
ENDIF("VTK_COMPILER_HAS_BOOL" MATCHES "^VTK_COMPILER_HAS_BOOL$")

IF("VTK_TYPE_CHAR_IS_SIGNED" MATCHES "^VTK_TYPE_CHAR_IS_SIGNED$")
  MESSAGE(STATUS "Checking signedness of char")
  TRY_RUN(VTK_TYPE_CHAR_IS_SIGNED VTK_TYPE_CHAR_IS_SIGNED_COMPILED
          ${VTK_BINARY_DIR}/CMakeTmp/Char
          ${VTK_CMAKE_DIR}/vtkTestCharSignedness.cxx)
  IF(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
    IF(VTK_TYPE_CHAR_IS_SIGNED)
      MESSAGE(STATUS "Checking signedness of char -- signed")
      SET(VTK_TYPE_CHAR_IS_SIGNED 1 CACHE INTERNAL "Whether char is signed.")
    ELSE(VTK_TYPE_CHAR_IS_SIGNED)
      MESSAGE(STATUS "Checking signedness of char -- unsigned")
      SET(VTK_TYPE_CHAR_IS_SIGNED 0 CACHE INTERNAL "Whether char is signed.")
    ENDIF(VTK_TYPE_CHAR_IS_SIGNED)
  ELSE(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
    MESSAGE(STATUS "Checking signedness of char -- failed")
  ENDIF(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
ENDIF("VTK_TYPE_CHAR_IS_SIGNED" MATCHES "^VTK_TYPE_CHAR_IS_SIGNED$")
