include(vtkMPI)

set(PCellSizeFilter_NUMPROCS 2)
vtk_add_test_mpi(vtkFiltersParallelVerdictCxxTests-MPI tests PCellSizeFilter.cxx,TESTING_DATA,NO_VALID)

vtk_test_mpi_executable(vtkFiltersParallelVerdictCxxTests-MPI tests
  PCellSizeFilter.cxx)
