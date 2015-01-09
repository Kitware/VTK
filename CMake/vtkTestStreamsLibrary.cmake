# Check the severity of EOF bugs in the streams library.
SET(VTK_TEST_STREAM_EOF_CXX ${VTK_CMAKE_DIR}/vtkTestStreamEOF.cxx.in)
CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkTestStreamEOF.cxx.in
  ${VTK_BINARY_DIR}/CMake/vtkTestStreamEOF.cxx @ONLY)
IF(NOT DEFINED VTK_ANSI_STREAM_EOF_RESULT)
  MESSAGE(STATUS "Checking ANSI streams end-of-file bug level")
  TRY_RUN(VTK_ANSI_STREAM_EOF_RESULT VTK_ANSI_STREAM_EOF_COMPILED
    ${VTK_BINARY_DIR}/CMakeTmp
    ${VTK_BINARY_DIR}/CMake/vtkTestStreamEOF.cxx)
  IF(VTK_ANSI_STREAM_EOF_COMPILED)
    MESSAGE(STATUS "Checking ANSI streams end-of-file bug level - ${VTK_ANSI_STREAM_EOF_RESULT}")
  ELSE()
    SET(VTK_ANSI_STREAM_EOF_RESULT 0)
    MESSAGE(STATUS "Checking ANSI streams end-of-file bug level - failed to compile test")
  ENDIF()
ENDIF()
SET(VTK_STREAM_EOF_SEVERITY ${VTK_ANSI_STREAM_EOF_RESULT})

IF(VTK_SIZEOF_LONG_LONG)
  CONFIGURE_FILE(${VTK_CMAKE_DIR}/vtkTestStreamLongLong.cxx.in
    ${VTK_BINARY_DIR}/CMake/vtkTestStreamLongLong.cxx @ONLY)
  IF(NOT DEFINED VTK_OSTREAM_SUPPORTS_LONG_LONG)
    MESSAGE(STATUS "Checking if ostream supports long long")
    TRY_COMPILE(VTK_OSTREAM_SUPPORTS_LONG_LONG
      ${VTK_BINARY_DIR}
      ${VTK_BINARY_DIR}/CMake/vtkTestStreamLongLong.cxx
      COMPILE_DEFINITIONS -DVTK_TEST_OSTREAM_LONG_LONG
      OUTPUT_VARIABLE OUTPUT)
    IF(VTK_OSTREAM_SUPPORTS_LONG_LONG)
      MESSAGE(STATUS "Checking if ostream supports long long -- yes")
      SET(VTK_OSTREAM_SUPPORTS_LONG_LONG 1 CACHE INTERNAL "Whether ostream supports long long")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Determining if ostream supports long long "
        "passed with the following output:\n"
        "${OUTPUT}\n")
    ELSE()
      MESSAGE(STATUS "Checking if ostream supports long long -- no")
      SET(VTK_OSTREAM_SUPPORTS_LONG_LONG 0 CACHE INTERNAL "Whether ostream supports long long")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining if ostream supports long long "
        "failed with the following output:\n"
        "${OUTPUT}\n")
    ENDIF()
  ENDIF()
  IF(NOT DEFINED VTK_ISTREAM_SUPPORTS_LONG_LONG)
    MESSAGE(STATUS "Checking if istream supports long long")
    TRY_COMPILE(VTK_ISTREAM_SUPPORTS_LONG_LONG
      ${VTK_BINARY_DIR}
      ${VTK_BINARY_DIR}/CMake/vtkTestStreamLongLong.cxx
      COMPILE_DEFINITIONS -DVTK_TEST_ISTREAM_LONG_LONG
      OUTPUT_VARIABLE OUTPUT)
    IF(VTK_ISTREAM_SUPPORTS_LONG_LONG)
      MESSAGE(STATUS "Checking if istream supports long long -- yes")
      SET(VTK_ISTREAM_SUPPORTS_LONG_LONG 1 CACHE INTERNAL "Whether istream supports long long")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
        "Determining if istream supports long long "
        "passed with the following output:\n"
        "${OUTPUT}\n")
    ELSE()
      MESSAGE(STATUS "Checking if istream supports long long -- no")
      SET(VTK_ISTREAM_SUPPORTS_LONG_LONG 0 CACHE INTERNAL "Whether istream supports long long")
      FILE(APPEND ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
        "Determining if istream supports long long "
        "failed with the following output:\n"
        "${OUTPUT}\n")
    ENDIF()
  ENDIF()
ENDIF()
