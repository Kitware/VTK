# check for gcc/clang atomic builtins like __atomic_add_fetch
if(NOT WIN32)
  if(NOT DEFINED VTK_HAVE_ATOMIC_BUILTINS)
    message(STATUS "Checking for builtin __atomic_add_fetch")
    try_compile(VTK_TEST_ATOMIC_BUILTINS_COMPILED
      ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/Sync
      ${CMAKE_CURRENT_SOURCE_DIR}/vtkTestSyncBuiltins.cxx
      OUTPUT_VARIABLE OUTPUT)
    if(VTK_TEST_ATOMIC_BUILTINS_COMPILED)
      set(vtk_atomic_add_fetch_detection "success")
      set(VTK_HAVE_ATOMIC_BUILTINS 1)
    else()
      set(vtk_atomic_add_fetch_detection "failed")
      set(VTK_HAVE_ATOMIC_BUILTINS 0)
    endif()
    message(STATUS "Checking for builtin __atomic_add_fetch -- ${vtk_atomic_add_fetch_detection}")
    set(VTK_HAVE_ATOMIC_BUILTINS ${VTK_HAVE_ATOMIC_BUILTINS}
      CACHE INTERNAL "For __atomic_ builtins.")
    file(APPEND "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log"
      "Determining if the C++ compiler supports __atomic_add_fetch builtin "
      "completed with the following output:\n"
      "${OUTPUT}\n")
  endif()
endif()
