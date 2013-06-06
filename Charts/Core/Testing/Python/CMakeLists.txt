if(VTK_PYTHON_EXE)
  # These tests should use vtk.test.Testing.
  set(tests
    TestBarGraph
    TestLinePlot
    TestStackedPlot
    TestLinePlotColors
    TestParallelCoordinatesColors
    TestScatterPlotColors
    )
  foreach(tfile ${tests})
    vtk_add_test_python(${tfile}.py NO_RT NO_OUTPUT)
  endforeach()
else()
  message(FATAL_ERROR "No Python tests added!")
endif()
