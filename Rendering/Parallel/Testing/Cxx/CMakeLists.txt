include(vtkMPI)

vtk_add_test_cxx(PrmMagnify.cxx)
vtk_test_cxx_executable(${vtk-module}CxxTests)

vtk_add_test_mpi(TestDistributedDataCompositeZPass.cxx TESTING_DATA)
vtk_add_test_mpi(TestPCompositeZPass.cxx TESTING_DATA)
vtk_add_test_mpi(TestPShadowMapPass.cxx TESTING_DATA)
vtk_add_test_mpi(TestParallelRendering.cxx)

vtk_mpi_link(TestDistributedDataCompositeZPass)
vtk_mpi_link(TestPCompositeZPass)
vtk_mpi_link(TestPShadowMapPass)
vtk_mpi_link(TestParallelRendering)

if (PYTHON_EXECUTABLE)
  vtk_module_test_executable(
    ${vtk-module}-TestClientServerRendering TestClientServerRendering.cxx)

  ExternalData_add_test(VTKData
    NAME ${vtk-module}-TestClientServerRendering
    COMMAND ${PYTHON_EXECUTABLE}
      ${VTK_CMAKE_DIR}/vtkTestDriver.py
      --process
        $<TARGET_FILE:${vtk-module}-TestClientServerRendering>
        --port 12367
        --server
      -D ${VTK_TEST_DATA_DIR}
        --process
        $<TARGET_FILE:${vtk-module}-TestClientServerRendering>
        --port 12367
        -D ${VTK_TEST_DATA_DIR}
        -T ${VTK_TEST_OUTPUT_DIR}
        -V "DATA{${${vtk-module}_SOURCE_DIR}/Testing/Data/Baseline/TestClientServerRendering.png,:}"
  )
endif()
