vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestConvertSelection.cxx,NO_VALID
  TestExtractSelection.cxx
  TestExtraction.cxx
  TestExtractRectilinearGrid.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
