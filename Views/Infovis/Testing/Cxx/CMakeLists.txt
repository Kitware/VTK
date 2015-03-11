if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(extra_opengl2_tests
    # the following produce incorrect results in OpenGL
    TestConeLayoutStrategy.cxx
    )
endif()

vtk_add_test_cxx(${vtk-module}CxxTests tests
  ${extra_opengl2_tests}
  TestCoincidentGraphLayoutView.cxx
  TestColumnTree.cxx
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
