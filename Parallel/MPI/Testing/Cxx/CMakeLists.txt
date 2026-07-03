vtk_add_test_mpi(vtkParallelMPICxxTests-MPI tests
  TESTING_DATA
  TestPProbe.cxx
  )
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI no_data_tests
  TestGenericCommunicator.cxx
  TestMPIController.cxx
  TestPDirectory.cxx
  TestPExecutableRunner.cxx
  TestPSystemTools.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 2_proc_tests
  TestNonBlockingCommunication.cxx
  TestProcess.cxx
  TestPartitionControllerByCount2Ranks,TestPartitionControllerByCount.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 3)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 3_proc_tests
  TestPartitionControllerByCount3Ranks,TestPartitionControllerByCount.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 4)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 4_proc_tests
  TestAllToAllVoidArray.cxx
  TestPartitionControllerByCount4Ranks,TestPartitionControllerByCount.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 5)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 5_proc_tests
  TestPartitionControllerByCount5Ranks,TestPartitionControllerByCount.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 6)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 6_proc_tests
  TestPartitionControllerByCount6Ranks,TestPartitionControllerByCount.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 7)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 7_proc_tests
  TestPartitionControllerByCount7Ranks,TestPartitionControllerByCount.cxx
  )

set(vtkParallelMPICxxTests-MPI_NUMPROCS 8)
vtk_add_test_mpi(vtkParallelMPICxxTests-MPI 8_proc_tests
  TestPartitionControllerByCount8Ranks,TestPartitionControllerByCount.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  ${2_proc_tests}
  ${3_proc_tests}
  ${4_proc_tests}
  ${5_proc_tests}
  ${6_proc_tests}
  ${7_proc_tests}
  ${8_proc_tests}
  )
vtk_test_cxx_executable(vtkParallelMPICxxTests-MPI all_tests
  TestExerciseMultiProcessController.cxx
  )
