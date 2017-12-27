include(vtkMPI)

vtk_add_test_mpi(vtkFiltersParallelMPICxxTests-MPI tests
  TestImplicitConnectivity.cxx
  )
vtk_test_mpi_executable(vtkFiltersParallelMPICxxTests-MPI tests)
