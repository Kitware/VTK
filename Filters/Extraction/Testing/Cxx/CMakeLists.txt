vtk_add_test_cxx(vtkFiltersExtractionCxxTests tests
  TestConvertSelection.cxx,NO_VALID
  TestExtractBlock.cxx,NO_VALID,NO_DATA
  TestExtractDataArraysOverTime.cxx,NO_VALID
  TestExtractThresholdsMultiBlock.cxx,NO_VALID
  TestExtraction.cxx
  TestExtractionExpression.cxx
  TestExtractRectilinearGrid.cxx,NO_VALID,NO_DATA
  TestExtractRows.cxx,NO_VALID,NO_DATA
  TestExtractSelectedArraysOverTime.cxx,NO_VALID
  TestExtractSelection.cxx
  TestExtractTimeSteps.cxx,NO_VALID
  )
vtk_test_cxx_executable(vtkFiltersExtractionCxxTests tests)
