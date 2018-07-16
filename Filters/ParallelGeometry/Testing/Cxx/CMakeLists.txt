include(vtkMPI)

set(vtkFiltersParallelGeometryCxxTests-MPI_NUMPROCS 4)
set(Tests_SRCS
  TestPStructuredGridConnectivity.cxx
  TestPStructuredGridGhostDataGenerator.cxx
  TestPUnstructuredGridGhostCellsGenerator.cxx
  TestPUniformGridGhostDataGenerator.cxx)

vtk_add_test_mpi(vtkFiltersParallelGeometryCxxTests-MPI tests ${Tests_SRCS})

set(vtkFiltersParallelGeometryCxxTests-MPI_NUMPROCS 1)
vtk_add_test_mpi(vtkFiltersParallelGeometryCxxTests-MPI data_tests_1_proc
  ParallelConnectivity1,ParallelConnectivity.cxx,TESTING_DATA,NO_VALID
  )

set(vtkFiltersParallelGeometryCxxTests-MPI_NUMPROCS 4)
vtk_add_test_mpi(vtkFiltersParallelGeometryCxxTests-MPI data_tests_4_procs
  ParallelConnectivity4,ParallelConnectivity.cxx,TESTING_DATA,NO_VALID
  )

set(all_tests
  ${tests}
  ${data_tests_1_proc}
  ${data_tests_4_procs}
)

vtk_test_mpi_executable(vtkFiltersParallelGeometryCxxTests-MPI all_tests
  UnstructuredGhostZonesCommon.cxx)
