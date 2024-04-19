vtk_add_test_cxx(vtkpngCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestIncludePNG.cxx)
vtk_test_cxx_executable(vtkpngCxxTests tests)
