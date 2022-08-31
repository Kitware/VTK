vtk_add_test_mpi(vtkFiltersParallelMPICxxTests-MPI tests
  TestImplicitConnectivity.cxx
  )

set(vtkFiltersParallelMPICxxTests-MPI_NUMPROCS 1)
vtk_add_test_mpi(vtkFiltersParallelMPICxxTests-MPI test_1_procs
  TestDistributedPointCloudFilter1,TestDistributedPointCloudFilter.cxx
  )

set(vtkFiltersParallelMPICxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkFiltersParallelMPICxxTests-MPI test_2_procs
  TestDistributedPointCloudFilter2,TestDistributedPointCloudFilter.cxx
  )

set(vtkFiltersParallelMPICxxTests-MPI_NUMPROCS 5)
vtk_add_test_mpi(vtkFiltersParallelMPICxxTests-MPI test_5_procs
  TestDistributedPointCloudFilter5,TestDistributedPointCloudFilter.cxx
  )

set(all_tests
  ${tests}
  ${test_4_procs}
  ${test_1_procs}
  ${test_2_procs}
  ${test_5_procs}
  )

vtk_test_cxx_executable(vtkFiltersParallelMPICxxTests-MPI all_tests)
