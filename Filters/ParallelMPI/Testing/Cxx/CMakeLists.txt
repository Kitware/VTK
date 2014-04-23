include(vtkMPI)

vtk_add_test_mpi(${vtk-module}CxxTests-MPI tests
  TestImplicitConnectivity.cxx
  )
vtk_test_mpi_executable(${vtk-module}CxxTests-MPI tests)
