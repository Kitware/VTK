IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

  IF(VTK_PYTHON_EXE)
    SET(tests)
    IF(VTK_DATA_ROOT)
      # Tests written in Python that require VTKData
      # These tests should use vtk.test.Testing.
      SET(tests
        ${tests}
        TestBarGraph
        TestLinePlot
        TestStackedPlot
        TestLinePlotColors
        TestParallelCoordinatesColors
        TestScatterPlotColors
      )
    ELSE()
      # Tests written in Python that do NOT require VTKData
      # These tests should use vtk.test.Testing.
      SET(tests
        ${tests}
      )
    ENDIF()
    IF(tests)
      FOREACH(tfile ${tests})
        ADD_TEST(Charts-${tfile}Python ${VTK_PYTHON_EXE}
          ${VTK_SOURCE_DIR}/Charts/Testing/Python/${tfile}.py
          -D ${VTK_DATA_ROOT}
          -B ${VTK_DATA_ROOT}/Baseline/Charts)
      ENDFOREACH()
    ENDIF()
  ENDIF()
ENDIF()
