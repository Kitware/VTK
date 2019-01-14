vtk_add_test_mpi(vtkIOMPIImageCxxTests-MPI tests
  TESTING_DATA
  ParallelIso.cxx
  ParallelIso2.cxx
  )
vtk_test_cxx_executable(vtkIOMPIImageCxxTests-MPI tests)
