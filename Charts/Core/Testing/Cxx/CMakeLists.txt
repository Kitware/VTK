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

vtk_add_test_cxx(vtkChartsCoreCxxTests tests
  TestAdjustLowerBoundForLogPlot.cxx
  TestContextScene.cxx,NO_DATA,NO_VALID
  TestColorTransferControlPointsItem.cxx,NO_DATA,NO_VALID
  TestCompositeControlPointsItem.cxx,NO_DATA,NO_VALID
  TestControlPointsItemEvents.cxx,NO_DATA,NO_VALID
  TestAreaPlot.cxx
  TestAxes.cxx,LOOSE_VALID
  TestAxes2.cxx
  TestBagPlot.cxx,LOOSE_VALID
  TestBarGraph.cxx,LOOSE_VALID
  TestBarGraphHorizontal.cxx,LOOSE_VALID
  TestBarGraphSelection.cxx,LOOSE_VALID
  TestBoxPlot.cxx,LOOSE_VALID
  TestBoxPlot2.cxx,LOOSE_VALID
  TestCategoryLegend.cxx
  TestColorTransferFunction.cxx
  TestChartBadPoints.cxx,LOOSE_VALID
  TestChartDouble.cxx
  TestChartDoubleColors.cxx
  TestChartDoubleColorsOpaque.cxx
  TestChartLogScaleUpdates.cxx
  TestChartMatrix.cxx,LOOSE_VALID
  TestChartMatrix2.cxx,LOOSE_VALID
  TestChartMatrix3.cxx
  TestChartPicking.cxx,NO_DATA,NO_VALID
  TestChartTileScaling.cxx
  TestChartUnicode.cxx
  TestChartsOn3D.cxx
  TestChartXYInvertedAxis.cxx
  TestChartXYZ.cxx
  TestChartXYZMarginsRotations.cxx
  TestChartXYZOuterEdgeLabelling.cxx
  TestChartXYZUnconstrained.cxx
  TestContext.cxx,LOOSE_VALID
  TestContextArea.cxx,LOOSE_VALID
  TestContextAreaFixedAspect.cxx,LOOSE_VALID
  TestContextAreaFixedMargins.cxx,LOOSE_VALID
  TestContextAreaFixedRect.cxx
  TestContextImage.cxx
  TestContextItemStacking.cxx
  TestContextUnicode.cxx
  TestControlPointsHandleItem.cxx
  TestDiagram.cxx
  TestFunctionalBagPlot.cxx,LOOSE_VALID
  TestHistogram2D.cxx,LOOSE_VALID
  TestHistogram2DComponent.cxx
  TestHistogram2DMagnitude.cxx
  TestInteractiveChartXYZ.cxx
  TestLegendHiddenPlots.cxx,LOOSE_VALID
  TestLinePlot.cxx
  TestLinePlotDouble.cxx
  TestLinePlotDouble2.cxx,LOOSE_VALID
  TestLinePlot3D.cxx
  TestLinePlotAxisFonts.cxx,LOOSE_VALID
  TestLinePlot2.cxx
  TestLinePlotInteraction.cxx
  TestLinePlotSelection.cxx,LOOSE_VALID
  TestLinePlotSelection2.cxx,LOOSE_VALID
  TestLinePlotSelectionMultiBlock.cxx
  TestMultipleChartRenderers.cxx,LOOSE_VALID
  TestMultipleRenderers.cxx
  TestMultipleScalarsToColors.cxx
  TestParallelCoordinates.cxx
  TestParallelCoordinatesDouble.cxx
  TestParallelCoordinatesLegend.cxx
  TestParallelCoordinatesSelection.cxx,NO_VALID
  TestPieChart.cxx
  TestPlotBarRangeHandlesItem.cxx,NO_DATA,NO_VALID
  TestPlotMatrix.cxx,LOOSE_VALID
  TestPlotRangeHandlesItem.cxx,NO_DATA,NO_VALID
  TestPropItem.cxx
  TestRangeHandlesItemEvents.cxx,NO_DATA,NO_VALID
  TestScalarsToColors.cxx
  TestScatterPlot.cxx,LOOSE_VALID
  TestScatterPlotMatrix.cxx
  TestScatterPlotMatrixHistogram.cxx
  TestScatterPlotMatrixVehicles.cxx
  TestScatterPlotMatrixVisible.cxx
  TestScientificPlot.cxx,LOOSE_VALID
  TestStackedBarGraph.cxx
  TestStackedPlot.cxx,LOOSE_VALID
  TestSurfacePlot.cxx
  TestZoomAxis.cxx
  TestZoomIndividualAxis.cxx
  )
vtk_test_cxx_executable(vtkChartsCoreCxxTests tests
  DISABLE_FLOATING_POINT_EXCEPTIONS
  RENDERING_FACTORY
  )
