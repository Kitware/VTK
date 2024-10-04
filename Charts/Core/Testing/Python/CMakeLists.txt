# These tests should use vtk.test.Testing.
vtk_add_test_python(
  TestBarGraph.py,LOOSE_VALID
  TestLinePlot.py,LOOSE_VALID
  TestStackedPlot.py,LOOSE_VALID
  TestLinePlotColors.py,LOOSE_VALID
  TestParallelCoordinatesColors.py,LOOSE_VALID
  TestScatterPlotColors.py,LOOSE_VALID
  )
