vtk_add_test_cxx(NO_VALID
  TestBiomTableReader.cxx
  TestNewickTreeReader.cxx
  TestMultiNewickTreeReader.cxx
  TestTulipReaderProperties.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
