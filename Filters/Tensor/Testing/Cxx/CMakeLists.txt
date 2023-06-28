# Tests with data
vtk_add_test_cxx(vtkFiltersTensorCxxTests tests
  TestTensorPrincipalInvariants.cxx,NO_VALID
  TestYieldCriteria.cxx,NO_VALID)

vtk_test_cxx_executable(vtkFiltersTensorCxxTests tests)
