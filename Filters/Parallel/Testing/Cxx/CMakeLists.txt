include(vtkMPI)

vtk_add_test_cxx(vtkFiltersParallelCxxTests testsStd
  TestAngularPeriodicFilter.cxx
  )

set(vtkFiltersParallelCxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI tests
  TESTING_DATA
  DistributedData.cxx
  DistributedDataRenderPass.cxx
  TransmitImageData.cxx
  TransmitImageDataRenderPass.cxx
  TransmitRectilinearGrid.cxx
  TransmitStructuredGrid.cxx
  )
vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI no_data_tests
  ParallelResampling.cxx
  UnitTestPMaskPoints.cxx
  )

set(vtkFiltersParallelCxxTests-MPI_NUMPROCS 1)
vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI data_tests_1_proc
  ParallelConnectivity1,ParallelConnectivity.cxx,TESTING_DATA,NO_VALID
  )

# We want 4 processes to test the vtkAggregateDataSetFilter
# properly
set(vtkFiltersParallelCxxTests-MPI_NUMPROCS 4)
vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI no_data_tests_4_procs
  AggregateDataSet.cxx
  )

vtk_add_test_mpi(vtkFiltersParallelCxxTests-MPI data_tests_4_procs
  ParallelConnectivity4,ParallelConnectivity.cxx,TESTING_DATA,NO_VALID
  )

set(all_tests
  ${tests}
  ${data_tests_1_proc}
  ${no_data_tests}
  ${no_data_tests_4_procs}
  ${data_tests_4_procs}
  )
vtk_test_mpi_executable(vtkFiltersParallelCxxTests-MPI all_tests)
vtk_test_cxx_executable(vtkFiltersParallelCxxTests testsStd)
