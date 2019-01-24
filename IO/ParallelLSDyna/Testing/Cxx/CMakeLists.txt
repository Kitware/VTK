
set(PLSDynaReader_NUMPROCS 2)
vtk_add_test_mpi(vtkIOParallelLSDynaCxxTests-MPI tests
  TESTING_DATA
  PLSDynaReader.cxx
  )
vtk_test_cxx_executable(vtkIOParallelLSDynaCxxTests-MPI tests)
