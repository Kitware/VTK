# check for gcc/clang atomic builtins like __sync_add_and_fetch
if(NOT WIN32)
  if(NOT DEFINED VTK_HAVE_SYNC_BUILTINS)
    message(STATUS "Checking for builtin __sync_add_and_fetch")
    try_compile(VTK_TEST_SYNC_BUILTINS_COMPILED
      ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/Sync
      ${CMAKE_CURRENT_SOURCE_DIR}/vtkTestSyncBuiltins.cxx
      OUTPUT_VARIABLE OUTPUT)
    if(VTK_TEST_SYNC_BUILTINS_COMPILED)
      set(vtk_sync_and_fetch_detection "success")
      set(vtk_have_sync_builtins 1)
    else()
      set(vtk_sync_and_fetch_detection "failed")
      set(vtk_have_sync_builtins 0)
    endif()
    message(STATUS "Checking for builtin __sync_add_and_fetch -- ${vtk_sync_and_fetch_detection}")
    set(VTK_HAVE_SYNC_BUILTINS ${vtk_have_sync_builtins}
      CACHE INTERNAL "For __sync atomic builtins.")
    file(APPEND "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log"
      "Determining if the C++ compiler supports __sync_add_and_fetch builtin "
      "completed with the following output:\n"
      "${OUTPUT}\n")
  endif()
endif()
