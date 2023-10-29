set(tests
  TestIncludeDIY2.cxx)

vtk_add_test_cxx(vtkdiy2CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkdiy2CxxTests tests)
