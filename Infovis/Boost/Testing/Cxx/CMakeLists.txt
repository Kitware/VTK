vtk_add_test_cxx(vtkInfovisBoostCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestVariantSerialization.cxx
  )
vtk_test_cxx_executable(vtkInfovisBoostCxxTests tests)
