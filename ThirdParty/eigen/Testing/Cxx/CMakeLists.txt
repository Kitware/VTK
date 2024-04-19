vtk_add_test_cxx(vtkeigenCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludeEigen.cxx)
vtk_test_cxx_executable(vtkeigenCxxTests tests)
