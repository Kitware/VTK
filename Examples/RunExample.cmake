cmake_minimum_required(VERSION 3.12)

if (WIN32)
  # Append the VTK DLL directory to PATH for the tests.
  list(APPEND ENV{PATH}
    "${vtk_binary_dir}")
endif ()

set(cmake_arguments)
if (platform)
  list(APPEND cmake_arguments
    -A "${platform}")
endif ()
if (toolset)
  list(APPEND cmake_arguments
    -T "${toolset}")
endif ()

execute_process(
  COMMAND
    "${ctest}"
    --build-generator
      "${generator}"
    --build-and-test
      "${source}/${example_dir}"
      "${binary}/${example_dir}"
    --build-options
      ${cmake_arguments}
      "-DBUILD_TESTING:BOOL=ON"
      "-DCMAKE_BUILD_TYPE:STRING=${build_type}"
      "-DBUILD_SHARED_LIBS:BOOL=${shared}"
      "-DVTK_DIR:PATH=${vtk_dir}"
    --test-command
      "${ctest}"
        -C ${config}
  RESULT_VARIABLE res)

if (res)
  message(FATAL_ERROR "Test command failed")
endif ()
