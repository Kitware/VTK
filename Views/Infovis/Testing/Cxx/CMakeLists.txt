vtk_add_test_cxx(Views
  TestCoincidentGraphLayoutView.cxx
  TestConeLayoutStrategy.cxx
  TestDendrogramOnly.cxx
  TestGraphLayoutView.cxx
  TestHeatmapOnly.cxx
  TestHierarchicalGraphView.cxx
  TestIcicleView.cxx
  TestInteractorStyleTreeMapHover.cxx
  TestNetworkViews.cxx
  TestRenderView.cxx
  TestSpanTreeLayoutStrategy.cxx
  TestTreeHeatmapAutoCollapse.cxx
  TestTreeHeatmapItem.cxx
  TestTreeMapView.cxx
  TestTreeRingView.cxx
  )

if( VTK_DATA_ROOT)
  vtk_add_test_cxx(TestIconGlyphFilter.cxx Views)
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
