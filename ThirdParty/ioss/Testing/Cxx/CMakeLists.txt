set(tests
  TestIncludeIOSS.cxx)

vtk_add_test_cxx(vtkiossCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkiossCxxTests tests)
