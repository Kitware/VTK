vtk_add_test_cxx(
  TestConvertSelection.cxx,NO_VALID
  TestExtractSelection.cxx
  TestExtraction.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests)
