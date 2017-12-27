vtk_add_test_cxx(vtkFiltersExtractionCxxTests tests
  TestConvertSelection.cxx,NO_VALID
  TestExtractSelection.cxx
  TestExtraction.cxx
  TestExtractRectilinearGrid.cxx,NO_VALID,NO_DATA
  TestExtractTimeSteps.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkFiltersExtractionCxxTests tests)
