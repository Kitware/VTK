set(tests
  TestIncludeLoguru.cxx)

vtk_add_test_cxx(vtkloguruCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkloguruCxxTests tests)
