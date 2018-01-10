include(vtkMPI)

vtk_add_test_cxx(vtkRenderingParallelCxxTests tests
  PrmMagnify.cxx
  )
vtk_test_cxx_executable(vtkRenderingParallelCxxTests tests)

set(vtkRenderingParallelCxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkRenderingParallelCxxTests-MPI tests
  TestSimplePCompositeZPass.cxx,TESTING_DATA
  TestParallelRendering.cxx,TESTING_DATA
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_mpi_executable(vtkRenderingParallelCxxTests-MPI all_tests)

if (PYTHON_EXECUTABLE)
  vtk_module_test_executable(
    vtkRenderingParallel-TestClientServerRendering TestClientServerRendering.cxx)

  ExternalData_add_test(VTKData
    NAME vtkRenderingParallel-TestClientServerRendering
    COMMAND ${PYTHON_EXECUTABLE}
      ${VTK_CMAKE_DIR}/vtkTestDriver.py
      --process
        $<TARGET_FILE:vtkRenderingParallel-TestClientServerRendering>
        --port 12367
        --server
      -D ${VTK_TEST_DATA_DIR}
        --process
        $<TARGET_FILE:vtkRenderingParallel-TestClientServerRendering>
        --port 12367
        -D ${VTK_TEST_DATA_DIR}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V "DATA{${CMAKE_CURRENT_SOURCE_DIR}/../Data/Baseline/TestClientServerRendering.png,:}"
  )
endif()
