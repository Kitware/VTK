SET(KIT Hybrid)
# add tests that do not require data
SET(MyTests     
  )
IF (VTK_DATA_ROOT)
  # add tests that require data
  SET(MyTests ${MyTests}
    )
ENDIF (VTK_DATA_ROOT)

# remove some tests if there is no display
IF(NOT VTK_USE_DISPLAY)
  REMOVE(MyTests     
    )
ENDIF(NOT VTK_USE_DISPLAY)

CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE vtkTestDriver.h)

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
