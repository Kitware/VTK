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
      add_test(NAME ${vtk-module}Python-${tfile}
        COMMAND ${VTK_PYTHON_EXE}
          ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
          -D ${VTK_DATA_ROOT}
          -B ${VTK_DATA_ROOT}/Baseline/Charts)
    endforeach()
  endif()
else()
  message(FATAL_ERROR "No Python tests added!")
endif()
