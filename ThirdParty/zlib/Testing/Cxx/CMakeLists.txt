set(tests
  TestIncludeZLIB.cxx)

vtk_add_test_cxx(vtkzlibCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkzlibCxxTests tests)
