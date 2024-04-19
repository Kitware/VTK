set(vtkIOParallelXdmf3CxxTests-MPI_NUMPROCS 2)
vtk_add_test_mpi(vtkIOParallelXdmf3CxxTests tests
  TestXdmf3Parallel.cxx,NO_VALID,TESTING_DATA
  )

vtk_test_cxx_executable(vtkIOParallelXdmf3CxxTests tests)
