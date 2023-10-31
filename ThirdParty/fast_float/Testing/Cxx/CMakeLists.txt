set(tests
  TestIncludeFastFloat.cxx)

vtk_add_test_cxx(vtkfast_floatCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkfast_floatCxxTests tests)
