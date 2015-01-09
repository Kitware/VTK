SET(VTK_SIZEOF_VOID_P  ${CMAKE_SIZEOF_VOID_P})

INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE(int      VTK_SIZEOF_INT)
CHECK_TYPE_SIZE(long     VTK_SIZEOF_LONG)
CHECK_TYPE_SIZE(char     VTK_SIZEOF_CHAR)
CHECK_TYPE_SIZE(short    VTK_SIZEOF_SHORT)
CHECK_TYPE_SIZE(float    VTK_SIZEOF_FLOAT)
CHECK_TYPE_SIZE(double   VTK_SIZEOF_DOUBLE)

CHECK_TYPE_SIZE("long long" VTK_SIZEOF_LONG_LONG)
CHECK_TYPE_SIZE("__int64"   VTK_SIZEOF___INT64)
CHECK_TYPE_SIZE("uintptr_t"   VTK_UINTPTR_T)

IF(VTK_SIZEOF___INT64)
  # In CMake 2.6 and above the type __int64 may have been found only
  # due to inclusion of a system header.  Further try-compiles using
  # the type should include the header too.
  SET(_HAVE_DEFS)
  FOREACH(def HAVE_SYS_TYPES_H HAVE_STDINT_H HAVE_STDDEF_H)
    IF(${def})
      LIST(APPEND _HAVE_DEFS -D${def})
    ENDIF()
  ENDFOREACH()

  IF(NOT DEFINED VTK_TYPE_SAME_LONG_AND___INT64)
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
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Determining whether long and __int64 are the same type "
        "passed with the following output:\n"
        "${OUTPUT}\n")
    ELSE()
      MESSAGE(STATUS "Checking whether long and __int64 are the same type -- no")
      SET(VTK_TYPE_SAME_LONG_AND___INT64 0 CACHE INTERNAL "Whether long and __int64 are the same type")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining whether long and __int64 are the same type "
        "failed with the following output:\n"
        "${OUTPUT}\n")
    ENDIF()
  ENDIF()
  IF(VTK_SIZEOF_LONG_LONG)
    IF(NOT DEFINED VTK_TYPE_SAME_LONG_LONG_AND___INT64)
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
        FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
          "Determining whether long long and __int64 are the same type "
          "passed with the following output:\n"
          "${OUTPUT}\n")
      ELSE()
        MESSAGE(STATUS "Checking whether long long and __int64 are the same type -- no")
        SET(VTK_TYPE_SAME_LONG_LONG_AND___INT64 0 CACHE INTERNAL "Whether long long and __int64 are the same type")
        FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
          "Determining whether long long and __int64 are the same type "
          "failed with the following output:\n"
          "${OUTPUT}\n")
      ENDIF()
    ENDIF()
  ENDIF()
  IF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
    IF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
#  VS 6 cannot convert unsigned __int64 to double unless the
# "Visual C++ Processor Pack" is installed.
      IF(NOT DEFINED VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
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
          FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
            "Determining whether unsigned __int64 can convert to double "
            "passed with the following output:\n"
            "${OUTPUT}\n")
        ELSE()
          MESSAGE(STATUS "Checking whether unsigned __int64 can convert to double -- no")
          SET(VTK_TYPE_CONVERT_UI64_TO_DOUBLE 0 CACHE INTERNAL "Whether unsigned __int64 can convert to double")
          FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
            "Determining whether unsigned __int64 can convert to double "
            "failed with the following output:\n"
            "${OUTPUT}\n")
        ENDIF()
      ENDIF()
    ENDIF()
  ENDIF()
ENDIF()

# Enable the "long long" type if it is available.  It is standard in
# C99 and C++03 but not in earlier standards.
SET(VTK_TYPE_USE_LONG_LONG)
IF(VTK_SIZEOF_LONG_LONG)
  SET(VTK_TYPE_USE_LONG_LONG 1)
ENDIF()

# Enable the "__int64" type if it is available and unique.  It is not
# standard.
SET(VTK_TYPE_USE___INT64)
IF(VTK_SIZEOF___INT64)
  IF(NOT VTK_TYPE_SAME_LONG_AND___INT64)
    IF(NOT VTK_TYPE_SAME_LONG_LONG_AND___INT64)
      SET(VTK_TYPE_USE___INT64 1)
    ENDIF()
  ENDIF()
ENDIF()

IF(NOT DEFINED VTK_TYPE_CHAR_IS_SIGNED)
  MESSAGE(STATUS "Checking signedness of char")
  TRY_RUN(VTK_TYPE_CHAR_IS_SIGNED VTK_TYPE_CHAR_IS_SIGNED_COMPILED
          ${VTK_BINARY_DIR}/CMakeTmp/Char
          ${VTK_CMAKE_DIR}/vtkTestCharSignedness.cxx)
  IF(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
    IF(VTK_TYPE_CHAR_IS_SIGNED)
      MESSAGE(STATUS "Checking signedness of char -- signed")
      SET(VTK_TYPE_CHAR_IS_SIGNED 1 CACHE INTERNAL "Whether char is signed.")
    ELSE()
      MESSAGE(STATUS "Checking signedness of char -- unsigned")
      SET(VTK_TYPE_CHAR_IS_SIGNED 0 CACHE INTERNAL "Whether char is signed.")
    ENDIF()
  ELSE()
    MESSAGE(STATUS "Checking signedness of char -- failed")
  ENDIF()
ENDIF()
