set(tests
  TestIncludeExpat.cxx)

vtk_add_test_cxx(vtkexpatCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkexpatCxxTests tests)
