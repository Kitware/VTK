if (TARGET VTK::ParallelMPI)
  vtk_add_test_cxx(vtkParallelDIYCxxTests-MPI tests
    TestDIYDataArraySerialization.cxx,NO_DATA NO_VALID)

  vtk_test_cxx_executable(vtkParallelDIYCxxTests tests)

  set(vtkParallelDIYCxxTests-MPI_NUMPROCS 3)
  vtk_add_test_mpi(vtkParallelDIYCxxTests-MPI tests
    NO_DATA
    TestDIYDataExchanger.cxx
    TestDIYUtilities.cxx)

  vtk_test_cxx_executable(vtkParallelDIYCxxTests-MPI tests)
endif()
