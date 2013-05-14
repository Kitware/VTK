if(VTK_PYTHON_EXE)
  set(tests)
  if(VTK_DATA_ROOT)
    # Tests written in Python that require VTKData
    # These tests should use vtk.test.Testing.
    set(tests
      ${tests}
      TestBarGraph
      TestLinePlot
      TestStackedPlot
      TestLinePlotColors
      TestParallelCoordinatesColors
      TestScatterPlotColors
    )
  else()
    # Tests written in Python that do NOT require VTKData
    # These tests should use vtk.test.Testing.
    set(tests
      ${tests}
    )
  endif()
  if(tests)
    foreach(tfile ${tests})
      vtk_add_test_python(${tfile}.py NO_RT NO_OUTPUT)
    endforeach()
  endif()
else()
  message(FATAL_ERROR "No Python tests added!")
endif()
