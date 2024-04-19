vtk_add_test_cxx(vtkConduitCxxTests tests
  NO_VALID NO_OUTPUT
  TestDataObjectToConduit.cxx
  TestConduitSource.cxx)

vtk_test_cxx_executable(vtkConduitCxxTests tests)

if (TARGET VTK::ParallelMPI)
  vtk_add_test_mpi(vtkConduitCxxTests-MPI mpitests
    TESTING_DATA NO_VALID NO_OUTPUT
    TestDataObjectToConduit.cxx
    TestConduitSource.cxx)

  vtk_test_cxx_executable(vtkConduitCxxTests-MPI mpitests)
endif()
