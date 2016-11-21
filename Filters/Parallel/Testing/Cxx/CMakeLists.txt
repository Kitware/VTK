include(vtkMPI)

vtk_add_test_cxx(${vtk-module}CxxTests testsStd
  TestAngularPeriodicFilter.cxx
  )

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TESTING_DATA
  DistributedData.cxx
  DistributedDataRenderPass.cxx
  TransmitImageData.cxx
  TransmitImageDataRenderPass.cxx
  TransmitRectilinearGrid.cxx
  TransmitStructuredGrid.cxx
  )
vtk_add_test_mpi(${vtk-module}CxxTests-MPI no_data_tests
  ParallelResampling.cxx
  UnitTestPMaskPoints.cxx
  )
# We want 4 processes to test the vtkAggregateDataSetFilter
# properly
set(${vtk-module}CxxTests-MPI_NUMPROCS 4)
vtk_add_test_mpi(${vtk-module}CxxTests-MPI no_data_tests_4_procs
  AggregateDataSet.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  ${no_data_tests_4_procs}
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI all_tests)
vtk_test_cxx_executable(${vtk-module}CxxTests testsStd)
