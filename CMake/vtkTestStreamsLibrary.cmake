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
