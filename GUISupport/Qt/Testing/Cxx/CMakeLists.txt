# if we have rendering add the following tests
SET(KIT QVTK)
# add tests that do not require data
SET(MyTests
  TestQtDebugLeaksView.cxx
  TestQtInitialization.cxx
  TestQtSQLDatabase.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  )
IF(VTK_USE_VIEWS)
  SET(MyTests
    ${MyTests}
    TestVtkQtTableView.cxx
    )
ENDIF(VTK_USE_VIEWS)

IF (VTK_DATA_ROOT)
  # add tests that require data
  SET(MyTests ${MyTests}

    )
ENDIF (VTK_DATA_ROOT)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE vtkTestDriver.h)
ADD_EXECUTABLE(${KIT}CxxTests ${Tests} QTestApp.cxx)
TARGET_LINK_LIBRARIES(${KIT}CxxTests QVTK)
SET (TestsToRun ${Tests})
LIST (REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  IF (VTK_DATA_ROOT)
    ADD_TEST(QVTK-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${KIT}/${TName}.png)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(QVTK-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
  ENDIF (VTK_DATA_ROOT)
ENDFOREACH (test)


# testing of vtkQtChart
if(VTK_USE_QTCHARTS)
  SET(Tests
    TestLineSeries.cxx
    TestBarChart.cxx
    TestBarSeriesColors.cxx
    TestStackedChart.cxx
    TestStackedChartAnimate.cxx
    TestStatisticalBoxChart.cxx
    PolylineItem.cxx
    PointMarkerItems.cxx
    Test1.cxx
    TestChartWidget.cxx
    )
  IF(VTK_USE_INFOVIS)
    SET(Tests ${Tests} TestOrderStatisticsBoxChart.cxx)
  ENDIF(VTK_USE_INFOVIS)

  ## Optionally add tests for vtkView subclasses
  IF(VTK_USE_VIEWS)
    SET(Tests ${Tests}
      TestVtkBarChartView.cxx
      TestVtkLineChartView.cxx
      TestVtkStackedChartView.cxx
      TestVtkStatisticalBoxChartView.cxx)
  ENDIF(VTK_USE_VIEWS)

  INCLUDE_DIRECTORIES(${vtkQtChart_SOURCE_DIR} ${vtkQtChart_BINARY_DIR})

  CREATE_TEST_SOURCELIST(TestSrcs QtChartCxxTests.cxx ${Tests} )

  ADD_EXECUTABLE(QtChartCxxTests ${TestSrcs} QTestApp.cxx)
  TARGET_LINK_LIBRARIES(QtChartCxxTests
    vtkQtChart
    vtkFiltering
    vtkCommon)

  IF(VTK_USE_INFOVIS)
    TARGET_LINK_LIBRARIES(QtChartCxxTests vtkInfovis)
  ENDIF(VTK_USE_INFOVIS)

  ## Optionally link against vtkViews
  IF(VTK_USE_VIEWS)
    TARGET_LINK_LIBRARIES(QtChartCxxTests QVTK vtkViews)
  ENDIF(VTK_USE_VIEWS)

  FOREACH(test ${Tests})
    GET_FILENAME_COMPONENT(testname ${test} NAME_WE)
    ADD_TEST(QtChart-${testname} ${CXX_TEST_PATH}/QtChartCxxTests ${testname} --exit)
  ENDFOREACH(test)

endif()
