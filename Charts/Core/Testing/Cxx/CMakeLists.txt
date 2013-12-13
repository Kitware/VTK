# add tests that do not require data
vtk_add_test_cxx(NO_DATA NO_VALID
  TestContextScene.cxx
  TestControlPointsItem.cxx
  TestControlPointsItemEvents.cxx
  )

set(TestChartUnicode_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})
set(TestContextUnicode_ARGS DATA{../Data/Fonts/DejaVuSans.ttf})

# add tests that require data
vtk_add_test_cxx(
  TestAxes.cxx
  TestBagPlot.cxx
  TestBarGraph.cxx
  TestBarGraphHorizontal.cxx
  TestCategoryLegend.cxx
  TestColorTransferFunction.cxx,-E80
  TestChartDouble.cxx
  TestChartMatrix.cxx
  TestChartUnicode.cxx,-E25
  TestChartsOn3D.cxx,-E16
  TestChartXYZ.cxx
  TestContext.cxx
  TestContextImage.cxx
  TestContextUnicode.cxx
  TestControlPointsHandleItem.cxx,-E30
  TestDiagram.cxx
  TestFunctionalBagPlot.cxx
  TestHistogram2D.cxx
  TestInteractiveChartXYZ.cxx
  TestLegendHiddenPlots.cxx
  TestLinePlot.cxx,-E25
  TestLinePlotDouble.cxx
  TestLinePlotDouble2.cxx
  TestLinePlot3D.cxx
  TestLinePlotAxisFonts.cxx
  TestLinePlot2.cxx,-E25
  TestLinePlotInteraction.cxx,-E25
  TestLinePlotSelection.cxx
  TestLinePlotSelection2.cxx
  TestMultipleChartRenderers.cxx
  TestMultipleRenderers.cxx,-E25
  TestMultipleScalarsToColors.cxx,-E25
  TestParallelCoordinates.cxx,-E15
  TestParallelCoordinatesDouble.cxx,-E15
  TestPieChart.cxx
  TestPlotMatrix.cxx
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

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
