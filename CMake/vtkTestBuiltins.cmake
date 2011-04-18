# check for gcc/clang atomic builtins like __sync_add_and_fetch
IF(NOT WIN32)
  IF("VTK_HAVE_SYNC_BUILTINS" MATCHES "^VTK_HAVE_SYNC_BUILTINS$")
  MESSAGE(STATUS "Checking for builtin __sync_add_and_fetch")
  TRY_COMPILE(VTK_TEST_SYNC_BUILTINS_COMPILED
    ${VTK_BINARY_DIR}/CMakeTmp/Sync
    ${VTK_CMAKE_DIR}/vtkTestSyncBuiltins.cxx
    OUTPUT_VARIABLE OUTPUT)
  IF(VTK_TEST_SYNC_BUILTINS_COMPILED)
    MESSAGE(STATUS "Checking for builtin __sync_add_and_fetch -- success")
    SET(VTK_HAVE_SYNC_BUILTINS 1 CACHE INTERNAL "For __sync atomic builtins.")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if the C++ compiler supports __sync_add_and_fetch builtin "
      "passed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ELSE(VTK_TEST_SYNC_BUILTINS_COMPILED)
    MESSAGE(STATUS "Checking for builtin __sync_add_and_fetch -- failed")
    SET(VTK_HAVE_SYNC_BUILTINS 0 CACHE INTERNAL "For __sync atomic builtins.")
    WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log
      "Determining if the C++ compiler supports __sync_add_and_fetch builtin "
      "failed with the following output:\n"
      "${OUTPUT}\n" APPEND)
  ENDIF(VTK_TEST_SYNC_BUILTINS_COMPILED)
  ENDIF("VTK_HAVE_SYNC_BUILTINS" MATCHES "^VTK_HAVE_SYNC_BUILTINS$")
ENDIF(NOT WIN32)
