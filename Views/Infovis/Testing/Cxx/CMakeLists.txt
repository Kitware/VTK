vtk_add_test_cxx(
  TestCoincidentGraphLayoutView.cxx
  TestConeLayoutStrategy.cxx
  TestDendrogramItem.cxx
  TestGraphLayoutView.cxx
  TestHeatmapItem.cxx
  TestHierarchicalGraphView.cxx
  TestIcicleView.cxx
  TestInteractorStyleTreeMapHover.cxx
  TestNetworkViews.cxx
  TestRenderView.cxx
  TestSpanTreeLayoutStrategy.cxx
  TestTanglegramItem.cxx
  TestTreeHeatmapAutoCollapse.cxx
  TestTreeHeatmapItem.cxx
  TestTreeMapView.cxx
  TestTreeRingView.cxx
  )

vtk_add_test_cxx(TestIconGlyphFilter.cxx)

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
