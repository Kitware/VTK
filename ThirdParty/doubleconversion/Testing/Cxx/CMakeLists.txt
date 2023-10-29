set(tests
  TestIncludeDoubleConversion.cxx)

vtk_add_test_cxx(vtkdoubleconversionCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT)
vtk_test_cxx_executable(vtkdoubleconversionCxxTests tests)
