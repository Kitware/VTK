set(tests
  TestIncludeLibHaru.cxx)

vtk_add_test_cxx(vtklibharuCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtklibharuCxxTests tests)
