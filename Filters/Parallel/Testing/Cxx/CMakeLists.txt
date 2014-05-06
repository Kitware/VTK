include(vtkMPI)

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
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI all_tests)
