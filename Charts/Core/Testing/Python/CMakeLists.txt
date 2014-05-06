# These tests should use vtk.test.Testing.
vtk_add_test_python(
  NO_RT NO_OUTPUT
  TestBarGraph.py
  TestLinePlot.py
  TestStackedPlot.py
  TestLinePlotColors.py
  TestParallelCoordinatesColors.py
  TestScatterPlotColors.py
  )
