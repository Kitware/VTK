set(tests
  TestIncludeGlew.cxx)

vtk_add_test_cxx(vtkglewCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkglewCxxTests tests)
