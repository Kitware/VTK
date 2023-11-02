vtk_add_test_cxx(vtkhdf5CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludeHDF5.cxx)
vtk_test_cxx_executable(vtkhdf5CxxTests tests)
