vtk_add_test_cxx(vtkFiltersModelingCxxTests tests
  TestButterflyScalars.cxx
  TestDijkstraGraphGeodesicPath.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestLinearCellExtrusion.cxx
  TestNamedColorsIntegration.cxx
  TestPolyDataPointSampler.cxx
  TestQuadRotationalExtrusion.cxx
  TestQuadRotationalExtrusionMultiBlock.cxx
  TestRotationalExtrusion.cxx
  TestSelectEnclosedPoints.cxx
  TestVolumeOfRevolutionFilter.cxx
  UnitTestCollisionDetectionFilter.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  UnitTestHausdorffDistancePointSetFilter.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  UnitTestSubdivisionFilters.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )
vtk_add_test_cxx(vtkFiltersModelingCxxTests tests
  TestCollisionDetectionAllContacts,TestCollisionDetection.cxx 0)
vtk_add_test_cxx(vtkFiltersModelingCxxTests tests
  TestCollisionDetectionFirstContact,TestCollisionDetection.cxx 1)
vtk_add_test_cxx(vtkFiltersModelingCxxTests tests
  TestCollisionDetectionHalfContacts,TestCollisionDetection.cxx 2)

vtk_test_cxx_executable(vtkFiltersModelingCxxTests tests RENDERING_FACTORY)
