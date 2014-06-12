vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestButterflyScalars.cxx
  TestNamedColorsIntegration.cxx
  TestPolyDataPointSampler.cxx
  TestQuadRotationalExtrusion.cxx
  TestQuadRotationalExtrusionMultiBlock.cxx
  TestRotationalExtrusion.cxx
  TestSelectEnclosedPoints.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
