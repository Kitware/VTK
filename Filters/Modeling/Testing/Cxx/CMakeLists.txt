vtk_add_test_cxx(vtkFiltersModelingCxxTests tests
  TestButterflyScalars.cxx
  TestNamedColorsIntegration.cxx
  TestPolyDataPointSampler.cxx
  TestQuadRotationalExtrusion.cxx
  TestQuadRotationalExtrusionMultiBlock.cxx
  TestRotationalExtrusion.cxx
  TestSelectEnclosedPoints.cxx
  TestVolumeOfRevolutionFilter.cxx
  UnitTestSubdivisionFilters.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )
vtk_test_cxx_executable(vtkFiltersModelingCxxTests tests)
