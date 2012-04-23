# if we have rendering add the following tests
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  SET(KIT Views)
  # add tests that do not require data
  SET(MyTests
    TestRenderView.cxx
    TestInteractorStyleTreeMapHover
    )
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests ${MyTests}
      TestCoincidentGraphLayoutView.cxx
      TestGraphLayoutView.cxx
      TestHierarchicalGraphView.cxx
      TestIconGlyphFilter.cxx
      TestTreeMapView.cxx
      TestTreeRingView.cxx
      TestIcicleView.cxx
      TestConeLayoutStrategy.cxx
      TestSpanTreeLayoutStrategy.cxx
      )

    IF (VTK_USE_QT)
      #add tests that require Qt
      INCLUDE(${QT_USE_FILE})

      SET(MyTests ${MyTests}
        TestQtTreeRingLabeler.cxx
        )
    ENDIF(VTK_USE_QT)
    
    # Add these classes only if vtkTypeUInt64Array exists
    IF(VTK_HAS_UINT64_ARRAY)
      SET(MyTests ${MyTests}
        TestNetworkViews.cxx
        )
    ENDIF(VTK_HAS_UINT64_ARRAY)
  ENDIF (VTK_DATA_ROOT)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
    EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkViews)

  IF(VTK_USE_QT)
    TARGET_LINK_LIBRARIES(${KIT}CxxTests ${QT_LIBRARIES})
  ENDIF(VTK_USE_QT)

  SET (TestsToRun ${Tests})
  LIST (REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)
  
  #
  # Add all the executables 
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(Views-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -T ${VTK_BINARY_DIR}/Testing/Temporary
        -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(Views-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test) 
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
