# if we have rendering and views add the following tests
IF(VTK_USE_RENDERING AND VTK_USE_VIEWS)

  SET(KIT Charts)
  # add tests that do not require data
  SET(MyTests
    TestContextScene.cxx
    TestControlPointsItem.cxx
    TestControlPointsItemEvents.cxx
    )

  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests ${MyTests}
      #TestContextUnicode.cxx # Disabled until Freetype unicode works
      TestBarGraph.cxx
      TestBarGraphHorizontal.cxx
      TestColorTransferFunction.cxx
      TestChartMatrix.cxx
      TestChartsOn3D.cxx
      TestContext.cxx
      TestContextImage.cxx
      TestControlPointsHandleItem.cxx
      TestDiagram.cxx
      #TestFreeTypeContextUnicode.cxx
      #TestFreeTypeRender.cxx
      TestHistogram2D.cxx
      TestLegendHiddenPlots.cxx
      TestLinePlot.cxx
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
      TestScatterPlotMatrixVisible.cxx
      TestScientificPlot.cxx
      TestStackedBarGraph.cxx
      TestStackedPlot.cxx
      )
    IF(VTK_USE_GLSL_SHADERS)
      SET(MyTests ${MyTests}
        TestGLSL.cxx
        )
    ENDIF(VTK_USE_GLSL_SHADERS)
    # Set the tolerance higher for a few tests that need it
    set(TestGLSLError 12)
    set(TestChartsOn3DError 16)
    SET(TestLinePlotError 25)
    SET(TestLinePlot2Error 25)
    SET(TestLinePlotInteractionError 25)
    SET(TestMultipleRenderersError 25)
    SET(TestMultipleScalarsToColorsError 25)
    SET(TestParallelCoordinatesError 15)
    SET(TestControlPointsHandleItemError 30)
  ENDIF (VTK_DATA_ROOT)

  IF (VTK_USE_QT)
    #add tests that require Qt
    INCLUDE(${QT_USE_FILE})
    IF (VTK_DATA_ROOT)
      SET(MyTests ${MyTests}
        #TestQtDiagram.cxx
        #TestQtContextUnicode.cxx
        )
      SET(TestQtDiagramError 25)
      SET(TestQtContextUnicodeError 30)
    ENDIF (VTK_DATA_ROOT)
  ENDIF(VTK_USE_QT)
  # Use the testing object factory, to reduce boilerplate code in tests.
  INCLUDE("${VTK_SOURCE_DIR}/Rendering/vtkTestingObjectFactory.cmake")
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkCharts)

  IF(VTK_USE_QT)
    TARGET_LINK_LIBRARIES(${KIT}CxxTests ${QT_LIBRARIES})
  ENDIF(VTK_USE_QT)

  SET (TestsToRun ${Tests})
  LIST(REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

  # Add all the executables
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      IF(${${TName}Error})
        SET(_error_threshold ${${TName}Error})
      ELSE()
        SET(_error_threshold 10)
      ENDIF()
      ADD_TEST(Charts-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${KIT}/${TName}.png
        -E ${_error_threshold})
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(Charts-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test)
ENDIF (VTK_USE_RENDERING AND VTK_USE_VIEWS)
