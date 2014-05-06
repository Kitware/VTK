include(vtkMPI)

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TestPStructuredGridConnectivity.cxx
  TestPStructuredGridGhostDataGenerator.cxx
  TestPUnstructuredGridConnectivity.cxx
  TestPUnstructuredGridGhostDataGenerator.cxx
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests
  UnstructuredGhostZonesCommon.cxx)
