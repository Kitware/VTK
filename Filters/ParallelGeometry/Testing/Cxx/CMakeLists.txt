include(vtkMPI)

set(Tests_SRCS
  TestPStructuredGridConnectivity.cxx
  TestPStructuredGridGhostDataGenerator.cxx
  TestPUnstructuredGridGhostCellsGenerator.cxx
  TestPUniformGridGhostDataGenerator.cxx)

set(TestPUnstructuredGridGhostCellsGenerator_NUMPROCS 4)

vtk_add_test_mpi(vtkFiltersParallelGeometryCxxTests-MPI tests ${Tests_SRCS})

vtk_test_mpi_executable(vtkFiltersParallelGeometryCxxTests-MPI tests
  UnstructuredGhostZonesCommon.cxx)
