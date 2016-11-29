include(vtkMPI)

set (${vtk-module}CxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TESTING_DATA
  TestPResampleToImage.cxx
  TestPResampleToImageCompositeDataSet.cxx
  TestPResampleWithDataSet.cxx
  TestPResampleWithDataSet2.cxx
)

vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests)
