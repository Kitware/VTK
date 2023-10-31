set(tests
  TestIncludeXDMF3.cxx)

vtk_add_test_cxx(vtkxdmf3CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkxdmf3CxxTests tests)
