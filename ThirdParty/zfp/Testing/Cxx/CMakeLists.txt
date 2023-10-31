set(tests
  TestIncludeZFP.cxx)

vtk_add_test_cxx(vtkzfpCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkzfpCxxTests tests)
