vtk_add_test_cxx(vtkIOCONVERGECFDCxxTests tests
  TestCONVERGECFDReader.cxx,NO_VALID
  TestCONVERGECFDReaderTime.cxx,NO_VALID
)
vtk_test_cxx_executable(vtkIOCONVERGECFDCxxTests tests)
