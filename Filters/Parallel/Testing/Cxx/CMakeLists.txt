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

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI all_tests)
vtk_test_cxx_executable(${vtk-module}CxxTests testsStd)
