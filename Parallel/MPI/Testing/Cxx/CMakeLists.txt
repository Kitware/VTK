include(vtkMPI)

vtk_add_test_mpi(vtkParallelMPICxxTests-MPI tests
  TESTING_DATA
  TestPProbe.cxx
  )
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI no_data_tests
  GenericCommunicator.cxx
  MPIController.cxx
  PDirectory.cxx
  PSystemTools.cxx
  TestNonBlockingCommunication.cxx
  TestProcess.cxx
  ${extra_opengl_tests}
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_mpi_executable(vtkParallelMPICxxTests-MPI all_tests
  ExerciseMultiProcessController.cxx
  )
