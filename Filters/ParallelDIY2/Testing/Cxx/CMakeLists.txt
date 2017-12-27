include(vtkMPI)

set (vtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkFiltersParallelDIY2CxxTests-MPI tests
  TESTING_DATA
  TestPResampleToImage.cxx
  TestPResampleToImageCompositeDataSet.cxx
  TestPResampleWithDataSet.cxx
  TestPResampleWithDataSet2.cxx
)

vtk_test_mpi_executable(vtkFiltersParallelDIY2CxxTests-MPI tests)
