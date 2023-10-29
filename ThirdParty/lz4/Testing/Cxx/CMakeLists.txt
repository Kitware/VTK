set(tests
  TestIncludeLZ4.cxx)

vtk_add_test_cxx(vtklz4CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtklz4CxxTests tests)
