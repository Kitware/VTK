vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestCategoricalColors.cxx,NO_VALID
  TestColorSeries.cxx
  TestColorSeriesLookupTables.cxx,NO_VALID
  TestNamedColors.cxx,NO_VALID
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
