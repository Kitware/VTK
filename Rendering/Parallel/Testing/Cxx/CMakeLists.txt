include(vtkMPI)

if(NOT VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  # all these tests use shadow maps for some odd reason
  set(extra_opengl_tests
    TestPCompositeZPass.cxx
    TestPShadowMapPass.cxx
    TestDistributedDataCompositeZPass.cxx
    )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  PrmMagnify.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  RENDERING_DATA
  TestSimplePCompositeZPass.cxx
  ${extra_opengl_tests}
  )
vtk_add_test_mpi(${vtk-module}CxxTests-MPI no_data_tests
  TestParallelRendering.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI all_tests)

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
