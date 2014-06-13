include(vtkMPI)

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TESTING_DATA
  TestPProbe.cxx
  )
vtk_add_test_mpi(${vtk-module}CxxTests-MPI no_data_tests
  TestDistributedDataShadowMapPass.cxx
  GenericCommunicator.cxx
  MPIController.cxx
  TestNonBlockingCommunication.cxx
  TestProcess.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI all_tests
  ExerciseMultiProcessController.cxx
  )
