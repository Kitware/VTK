include(vtkMPI)

if(NOT VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(extra_opengl_tests
    TestDistributedDataShadowMapPass.cxx
    )
endif()

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TESTING_DATA
  TestPProbe.cxx
  )
vtk_add_test_mpi(${vtk-module}CxxTests-MPI no_data_tests
  GenericCommunicator.cxx
  MPIController.cxx
  TestNonBlockingCommunication.cxx
  TestProcess.cxx
  ${extra_opengl_tests}
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI all_tests
  ExerciseMultiProcessController.cxx
  )
