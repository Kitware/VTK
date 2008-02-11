# if we have rendering add the following tests
SET(KIT QVTK)
# add tests that do not require data
SET(MyTests
  TestQtInitialization.cxx
  TestQtSQLDatabase.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx  
  )
IF (VTK_DATA_ROOT)
  # add tests that require data
  SET(MyTests ${MyTests}
    )
ENDIF (VTK_DATA_ROOT)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE vtkTestDriver.h)
ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests QVTK)
SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

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
