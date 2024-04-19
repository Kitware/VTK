vtk_add_test_cxx(vtkmpiCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludeMPI.cxx)
vtk_test_cxx_executable(vtkmpiCxxTests tests)
