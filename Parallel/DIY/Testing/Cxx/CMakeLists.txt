if (TARGET VTK::ParallelMPI)
  set(vtkParallelDIYCxxTests-MPI_NUMPROCS 3)
  vtk_add_test_mpi(vtkParallelDIYCxxTests-MPI tests
    NO_DATA
    TestDIYDataExchanger.cxx
    TestDIYUtilities.cxx)

  vtk_test_cxx_executable(vtkParallelDIYCxxTests-MPI tests)
endif()
