vtk_add_test_cxx(vtkCommonColorCxxTests tests
  TestCategoricalColors.cxx,NO_VALID
  TestColorSeries.cxx
  TestColorSeriesLookupTables.cxx,NO_VALID
  TestNamedColors.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkCommonColorCxxTests tests)
