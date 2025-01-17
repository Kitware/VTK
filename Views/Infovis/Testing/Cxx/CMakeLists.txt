vtk_add_test_cxx(vtkViewsInfovisCxxTests tests
  TestConeLayoutStrategy.cxx
  TestCoincidentGraphLayoutView.cxx
  TestColumnTree.cxx
  TestDendrogramItem.cxx
  TestGraphLayoutView.cxx
  TestHeatmapCategoryLegend.cxx
  TestHeatmapItem.cxx,LOOSE_VALID
  TestHeatmapScalarLegend.cxx
  TestHierarchicalGraphView.cxx
  TestIcicleView.cxx
  TestIconGlyphFilter.cxx
  TestInteractorStyleTreeMapHover.cxx
  TestNetworkViews.cxx
  TestParallelCoordinatesView.cxx,LOOSE_VALID
  TestRenderView.cxx
  TestSpanTreeLayoutStrategy.cxx
  TestTanglegramItem.cxx
  TestTreeHeatmapAutoCollapse.cxx,LOOSE_VALID
  TestTreeHeatmapItem.cxx,LOOSE_VALID
  TestTreeMapView.cxx,LOOSE_VALID
  TestTreeRingView.cxx
  )

vtk_test_cxx_executable(vtkViewsInfovisCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY)
