if (TARGET VTK::ParallelMPI)
  set(vtkIOFidesCxxTests-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkIOFidesCxxTests-MPI tests
    TESTING_DATA
    TestFidesGhostCells.cxx,NO_VALID)
  vtk_test_cxx_executable(vtkIOFidesCxxTests-MPI tests)
endif()
