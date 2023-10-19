if(TARGET VTK::ParallelMPI)
  set(vtkPIOReaderTests-MPI_NUMPROCS 2)
  vtk_add_test_mpi(vtkPIOReaderTests all_tests
    TESTING_DATA
    TestPIOReaderHDF5.cxx
    )
  vtk_test_cxx_executable(vtkPIOReaderTests all_tests)
endif()
