vtk_add_test_cxx(vtkRenderingParallelCxxTests tests
  PrmMagnify.cxx
  )
vtk_test_cxx_executable(vtkRenderingParallelCxxTests tests)

if(TARGET VTK::ParallelMPI)
  set(vtkRenderingParallelCxxTests-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkRenderingParallelCxxTests-MPI tests
    TestSimplePCompositeZPass.cxx,TESTING_DATA
    TestParallelRendering.cxx,TESTING_DATA
    )
endif()

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_cxx_executable(vtkRenderingParallelCxxTests-MPI all_tests)

if (PYTHON_EXECUTABLE)
  vtk_module_test_executable(
    vtkRenderingParallel-TestClientServerRendering TestClientServerRendering.cxx)

  ExternalData_add_test(${_vtk_build_TEST_DATA_TARGET}
    NAME vtkRenderingParallel-TestClientServerRendering
    COMMAND ${PYTHON_EXECUTABLE}
      ${VTK_SOURCE_DIR}/CMake/vtkTestDriver.py
      --process
        $<TARGET_FILE:vtkRenderingParallel-TestClientServerRendering>
        --port 12367
        --server
      -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
        --process
        $<TARGET_FILE:vtkRenderingParallel-TestClientServerRendering>
        --port 12367
        -D ${_vtk_build_TEST_OUTPUT_DATA_DIRECTORY}
        -T ${_vtk_build_TEST_OUTPUT_DIRECTORY}
        -V "DATA{../Data/Baseline/TestClientServerRendering.png,:}"
  )
endif()
