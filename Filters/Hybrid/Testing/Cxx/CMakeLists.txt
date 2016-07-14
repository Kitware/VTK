vtk_add_test_cxx(${vtk-module}CxxTests tests
  TemporalStatistics.cxx
  TestBSplineTransform.cxx
  TestDepthSortPolyData.cxx
  TestForceTime.cxx
  TestPolyDataSilhouette.cxx
  TestProcrustesAlignmentFilter.cxx,NO_VALID
  TestTemporalCacheSimple.cxx,NO_VALID
  TestTemporalCacheTemporal.cxx,NO_VALID
  TestTemporalFractal.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )
