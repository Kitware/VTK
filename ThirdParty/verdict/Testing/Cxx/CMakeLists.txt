set(tests
  TestIncludeVerdict.cxx)

vtk_add_test_cxx(vtkverdictCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkverdictCxxTests tests)
