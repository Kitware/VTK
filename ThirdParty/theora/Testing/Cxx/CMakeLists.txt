set(tests
  TestIncludeTheora.cxx)

vtk_add_test_cxx(vtktheoraCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtktheoraCxxTests tests)
