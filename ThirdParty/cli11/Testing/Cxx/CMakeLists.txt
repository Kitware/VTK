set(tests
  TestIncludeCLI11.cxx)

vtk_add_test_cxx(vtkcli11CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkcli11CxxTests tests)
