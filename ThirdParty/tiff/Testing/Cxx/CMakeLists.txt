vtk_add_test_cxx(vtktiffCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludeTIFF.cxx)
vtk_test_cxx_executable(vtktiffCxxTests tests)
