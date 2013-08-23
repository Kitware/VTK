# add tests that do not require data
vtk_add_test_cxx(NO_DATA NO_VALID
  TestContextScene.cxx
  TestControlPointsItem.cxx
  TestControlPointsItemEvents.cxx
  )

# add tests that require data
vtk_add_test_cxx(
  TestAxes.cxx
  TestBarGraph.cxx
  TestBarGraphHorizontal.cxx
  TestColorTransferFunction.cxx,-E80
  TestChartMatrix.cxx
  TestChartsOn3D.cxx,-E16
  TestChartXYZ.cxx
  TestContext.cxx
  TestContextImage.cxx
  TestControlPointsHandleItem.cxx,-E30
  TestDiagram.cxx
  TestHistogram2D.cxx
  TestInteractiveChartXYZ.cxx
  TestLegendHiddenPlots.cxx
  TestLinePlot.cxx,-E25
  TestLinePlotDouble.cxx
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
  )

vtk_test_cxx_executable(${vtk-module}CxxTests RENDERING_FACTORY)
