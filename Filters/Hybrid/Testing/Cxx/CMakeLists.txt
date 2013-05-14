vtk_add_test_cxx(
  # TestBSplineWarp.cxx           # Fixme after vtkImageViewer deps
  TestPolyDataSilhouette.cxx
  TestTemporalCacheSimple.cxx,NO_VALID
  TestTemporalCacheTemporal.cxx,NO_VALID
  )
vtk_add_test_cxx(
  TestTemporalFractal.cxx
  )
vtk_add_test_cxx(
  TemporalStatistics.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
