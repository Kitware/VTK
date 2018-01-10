include(vtkMPI)

set(vtkFiltersParallelMPICxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkFiltersParallelMPICxxTests-MPI tests
  TestImplicitConnectivity.cxx
  )
vtk_test_mpi_executable(vtkFiltersParallelMPICxxTests-MPI tests)
