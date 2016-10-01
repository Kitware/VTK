set(TestChartUnicode_ARGS
  DATA{../Data/Fonts/DejaVuSans.ttf}
  -E 25
  )
set(TestChartsOn3D_ARGS
  -E 16
  )
set(TestColorTransferFunction_ARGS
  -E 80
  )
set(TestContextUnicode_ARGS
  DATA{../Data/Fonts/DejaVuSans.ttf}
  )
set(TestControlPointsHandleItem_ARGS
  -E 30
  )
set(TestLinePlot_ARGS
  -E 25
  )
set(TestLinePlot2_ARGS
  -E 25
  )
set(TestLinePlotInteraction_ARGS
  -E 25
  )
set(TestMultipleRenderers_ARGS
  -E 25
  )
set(TestMultipleScalarsToColors_ARGS
  -E 25
  )
set(TestParallelCoordinates_ARGS
  -E 15
  )
set(TestParallelCoordinatesDouble_ARGS
  -E 15
  )

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestContextScene.cxx,NO_DATA,NO_VALID
  TestControlPointsItem.cxx,NO_DATA,NO_VALID
  TestControlPointsItemEvents.cxx,NO_DATA,NO_VALID
  TestAreaPlot.cxx
  TestAxes.cxx
  TestBagPlot.cxx
  TestBarGraph.cxx
  TestBarGraphHorizontal.cxx
  TestBarGraphSelection.cxx
  TestBoxPlot.cxx
  TestBoxPlot2.cxx
  TestCategoryLegend.cxx
  TestColorTransferFunction.cxx
  TestChartDouble.cxx
  TestChartDoubleColors.cxx
  TestChartMatrix.cxx
  TestChartTileScaling.cxx
  TestChartUnicode.cxx
  TestChartsOn3D.cxx
  TestChartXYInvertedAxis.cxx
  TestChartXYZ.cxx
  TestContext.cxx
  TestContextArea.cxx
  TestContextAreaFixedAspect.cxx
  TestContextAreaFixedMargins.cxx
  TestContextAreaFixedRect.cxx
  TestContextImage.cxx
  TestContextItemStacking.cxx
  TestContextUnicode.cxx
  TestControlPointsHandleItem.cxx
  TestDiagram.cxx
  TestFunctionalBagPlot.cxx
  TestHistogram2D.cxx
  TestInteractiveChartXYZ.cxx
  TestLegendHiddenPlots.cxx
  TestLinePlot.cxx
  TestLinePlotDouble.cxx
  TestLinePlotDouble2.cxx
  TestLinePlot3D.cxx
  TestLinePlotAxisFonts.cxx
  TestLinePlot2.cxx
  TestLinePlotInteraction.cxx
  TestLinePlotSelection.cxx
  TestLinePlotSelection2.cxx
  TestMultipleChartRenderers.cxx
  TestMultipleRenderers.cxx
  TestMultipleScalarsToColors.cxx
  TestParallelCoordinates.cxx
  TestParallelCoordinatesDouble.cxx
  TestPieChart.cxx
  TestPlotMatrix.cxx
  TestPropItem.cxx
  TestScalarsToColors.cxx
  TestScatterPlot.cxx
  TestScatterPlotMatrix.cxx
  TestScatterPlotMatrixVehicles.cxx
  TestScatterPlotMatrixVisible.cxx
  TestScientificPlot.cxx
  TestStackedBarGraph.cxx
  TestStackedPlot.cxx
  TestSurfacePlot.cxx
  TestZoomAxis.cxx
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  RENDERING_FACTORY
  )
