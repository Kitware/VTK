SET(KIT Hybrid)
# add tests that do not require data
SET(MyTests     
  BoxWidget.cxx
  TestImplicitPlaneWidget.cxx
  )
IF (VTK_DATA_ROOT)
  # add tests that require data
  SET(MyTests ${MyTests}     
    ImagePlaneWidget.cxx
    PushTest.cxx
    TestSplineWidget.cxx 
    TestScalarBarWidget.cxx
    TestLineWidget.cxx
    TestPlaneWidget.cxx
    TestPointWidget.cxx    
    )
ENDIF (VTK_DATA_ROOT)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests})

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkHybrid)
SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables 
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  IF (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
        -D ${VTK_DATA_ROOT}
        -V Baseline/${KIT}/${TName}.png)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
  ENDIF (VTK_DATA_ROOT)
ENDFOREACH (test) 
