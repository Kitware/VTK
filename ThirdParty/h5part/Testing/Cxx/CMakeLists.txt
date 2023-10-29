set(tests
  TestIncludeH5Part.cxx)

vtk_add_test_cxx(vtkh5partCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkh5partCxxTests tests)
