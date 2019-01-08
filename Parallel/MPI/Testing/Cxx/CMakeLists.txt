vtk_add_test_mpi(vtkParallelMPICxxTests-MPI tests
  TESTING_DATA
  TestPProbe.cxx
  )
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI no_data_tests
  #GenericCommunicator.cxx
  MPIController.cxx
  PDirectory.cxx
  PSystemTools.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 2_proc_tests
  TestNonBlockingCommunication.cxx
  TestProcess.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  ${2_proc_tests}
  )
vtk_test_cxx_executable(vtkParallelMPICxxTests-MPI all_tests
  ExerciseMultiProcessController.cxx
  )
