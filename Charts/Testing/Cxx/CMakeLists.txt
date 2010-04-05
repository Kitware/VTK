# if we have rendering and views add the following tests
IF (VTK_USE_RENDERING AND VTK_USE_VIEWS)
  SET(KIT Charts)
  # add tests that do not require data
  SET(MyTests
    TestPCPlot.cxx
    TestDiagram.cxx
    TestLinePlot.cxx
    TestContext.cxx
    TestVector.cxx
    )
    
  IF (VTK_DATA_ROOT)
    # add tests that require data
#    SET(MyTests ${MyTests}
#      TestSpanTreeLayoutStrategy.cxx
#      )
  ENDIF (VTK_DATA_ROOT)

  IF (VTK_USE_QT)
    #add tests that require Qt
    INCLUDE(${QT_USE_FILE})
    SET(MyTests ${MyTests}
      TestQtDiagram.cxx
      )
    ENDIF(VTK_USE_QT)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
    EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkCharts)

  IF(VTK_USE_QT)
    TARGET_LINK_LIBRARIES(${KIT}CxxTests ${QT_LIBRARIES})
  ENDIF(VTK_USE_QT)

  SET (TestsToRun ${Tests})
  REMOVE (TestsToRun ${KIT}CxxTests.cxx)
  
  # Add all the executables 
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(Charts-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(Charts-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test) 
ENDIF (VTK_USE_RENDERING AND VTK_USE_VIEWS)
