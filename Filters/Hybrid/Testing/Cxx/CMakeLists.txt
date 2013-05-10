vtk_add_test_cxx(Hybrid
  # TestBSplineWarp.cxx           # Fixme after vtkImageViewer deps
  TestPolyDataSilhouette.cxx
  TestTemporalCacheSimple.cxx
  TestTemporalCacheTemporal.cxx
  )
vtk_add_test_cxx(Parallel
  TestTemporalFractal.cxx
  )
vtk_add_test_cxx(Graphics
  TemporalStatistics.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
