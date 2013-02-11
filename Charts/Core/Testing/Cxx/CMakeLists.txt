# if we have rendering and views add the following tests
  # add tests that do not require data
  set(MyTests
    TestContextScene.cxx
    TestControlPointsItem.cxx
    TestControlPointsItemEvents.cxx
    )

  if(VTK_DATA_ROOT)
    # add tests that require data
    set(MyTests ${MyTests}
      TestAxes.cxx
      TestBarGraph.cxx
      TestBarGraphHorizontal.cxx
      TestColorTransferFunction.cxx
      TestChartMatrix.cxx
      TestChartsOn3D.cxx
      TestChartXYZ.cxx
      TestContext.cxx
      TestContextImage.cxx
      TestControlPointsHandleItem.cxx
      TestDiagram.cxx
      TestHistogram2D.cxx
      TestInteractiveChartXYZ.cxx
      TestLegendHiddenPlots.cxx
      TestLinePlot.cxx
      TestLinePlot3D.cxx
      TestLinePlotAxisFonts.cxx
      TestLinePlot2.cxx
      TestLinePlotInteraction.cxx
      TestMultipleChartRenderers.cxx
      TestMultipleRenderers.cxx
      TestMultipleScalarsToColors.cxx
      TestParallelCoordinates.cxx
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
    # Set the tolerance higher for a few tests that need it
    set(TestGLSLError 12)
    set(TestChartsOn3DError 16)
    set(TestLinePlotError 25)
    set(TestLinePlot2Error 25)
    set(TestLinePlotInteractionError 25)
    set(TestMultipleRenderersError 25)
    set(TestMultipleScalarsToColorsError 25)
    set(TestParallelCoordinatesError 15)
    set(TestControlPointsHandleItemError 30)
    set(TestColorTransferFunctionError 80)
  endif()

  # Use the testing object factory, to reduce boilerplate code in tests.
  include(vtkTestingObjectFactory)
  vtk_module_test_executable(${vtk-module}CxxTests ${Tests})

  set(TestsToRun ${Tests})
  list(REMOVE_ITEM TestsToRun CxxTests.cxx)

  # Add all the executables
  foreach(test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    if(VTK_DATA_ROOT)
      if(${${TName}Error})
        set(_error_threshold ${${TName}Error})
      else()
        set(_error_threshold 10)
      endif()
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_TEST_OUTPUT_DIR}
          -V Baseline/Charts/${TName}.png
          -E ${_error_threshold})
    else()
      add_test(NAME ${vtk-module}Cxx-${TName}
        COMMAND ${vtk-module}CxxTests ${TName})
    endif()
  endforeach()
