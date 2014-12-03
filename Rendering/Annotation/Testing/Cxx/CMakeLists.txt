vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestAxisActor3D.cxx
  TestBarChartActor.cxx
  TestCaptionActor2D.cxx
  TestConvexHull2D.cxx,NO_VALID
  TestCornerAnnotation.cxx
  TestCubeAxes2DMode.cxx
  TestCubeAxes3.cxx
  TestCubeAxesWithGridLines.cxx
  TestCubeAxesWithXInnerGrids.cxx
  TestCubeAxesWithXInnerPolys.cxx
  TestCubeAxesWithXLines.cxx
  TestCubeAxesWithYInnerGrids.cxx
  TestCubeAxesWithYInnerPolys.cxx
  TestCubeAxesWithYLines.cxx
  TestCubeAxesWithZInnerGrids.cxx
  TestCubeAxesWithZInnerPolys.cxx
  TestCubeAxesWithZLines.cxx
  TestLegendBoxActor.cxx
  TestLegendScaleActor.cxx
  TestPieChartActor.cxx
  TestPolarAxes.cxx
  TestScalarBar.cxx
  TestSpiderPlotActor.cxx
  TestCubeAxesOrientedBoundingBox.cxx
  TestCubeAxesIntersectionPoint.cxx
  TestCubeAxesInnerGridAll.cxx
  TestCubeAxesInnerGridClosest.cxx
  TestCubeAxesInnerGridFurthest.cxx
  TestCubeAxesSticky.cxx
  TestCubeAxesStickyCentered.cxx
  TestXYPlotActor.cxx
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY)
