set(tests
  TestIncludeFreetype.cxx)

vtk_add_test_cxx(vtkfreetypeCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkfreetypeCxxTests tests)
