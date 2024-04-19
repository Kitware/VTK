vtk_add_test_cxx(vtkFiltersHybridCxxTests tests
  TemporalStatistics.cxx
  TestHyperTreeGridBinary2DAdaptiveDataSetSurfaceFilter.cxx
  TestHyperTreeGridBinary2DAdaptiveDataSetSurfaceFilterMaterial.cxx
  TestHyperTreeGridTernary3DAdaptiveDataSetSurfaceFilter.cxx
  TestHyperTreeGridTernary3DToUnstructuredAdaptiveDataSetSurfaceFilter.cxx
  TestHyperTreeGridTernary3DAdaptiveDataSetSurfaceFilterMaterial.cxx
  TestBSplineTransform.cxx
  TestDepthSortPolyData.cxx
  TestForceTime.cxx
  TestGenerateTimeSteps.cxx,NO_VALID
  TestPolyDataSilhouette.cxx
  TestProcrustesAlignmentFilter.cxx,NO_VALID
  TestTemporalArrayOperatorFilter.cxx,NO_VALID
  TestTemporalCacheSimple.cxx,NO_VALID
  TestTemporalCacheTemporal.cxx,NO_VALID
  TestTemporalCacheMemkind.cxx,NO_VALID
  TestTemporalCacheUndefinedTimeStep.cxx
  TestTemporalFractal.cxx
  TestTemporalInterpolator.cxx
  TestTemporalInterpolatorFactorMode.cxx
  )
vtk_test_cxx_executable(vtkFiltersHybridCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
