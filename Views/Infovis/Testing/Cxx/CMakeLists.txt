vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestCoincidentGraphLayoutView.cxx
  TestColumnTree.cxx
  TestConeLayoutStrategy.cxx
  TestDendrogramItem.cxx
  TestGraphLayoutView.cxx
  TestHeatmapCategoryLegend.cxx
  TestHeatmapItem.cxx
  TestHeatmapScalarLegend.cxx
  TestHierarchicalGraphView.cxx
  TestIcicleView.cxx
  TestIconGlyphFilter.cxx
  TestInteractorStyleTreeMapHover.cxx
  TestNetworkViews.cxx
  TestParallelCoordinatesView.cxx
  TestRenderView.cxx
  TestSpanTreeLayoutStrategy.cxx
  TestTanglegramItem.cxx
  TestTreeHeatmapAutoCollapse.cxx
  TestTreeHeatmapItem.cxx
  TestTreeMapView.cxx
  TestTreeRingView.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY)
