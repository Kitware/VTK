include(vtkMPI)

vtk_add_test_mpi(vtkFiltersParallelMPICxxTests-MPI tests
  TestImplicitConnectivity.cxx
  )

# need 5 procs to test use of mpi split
set(${vtk-module}CxxTests-MPI_NUMPROCS 5)
vtk_add_test_mpi(${vtk-module}CxxTests-MPI test_5_procs
  TestDistributedPointCloudFilter.cxx
  )

set(all_tests
  ${tests}
  ${test_5_procs}
  )

vtk_test_mpi_executable(vtkFiltersParallelMPICxxTests-MPI all_tests)
