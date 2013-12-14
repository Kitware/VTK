vtk_add_test_cxx(
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

vtk_add_test_cxx(TestIconGlyphFilter.cxx)

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
