# if we have rendering add the following tests
IF (VTK_USE_DISPLAY)
  SET(KIT Geovis)
  # add tests that do not require data
  SET(MyTests
    TestGeoAssignCoordinates.cxx
    )
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests ${MyTests}
      TestCoincidentGeoGraphRepresentation2D.cxx
      TestGeoGraticule.cxx
      TestGeoProjection.cxx
      TestGeoView2D.cxx
      TestGeoView.cxx
      TestLabeledGeoView2D.cxx
      )
  ENDIF (VTK_DATA_ROOT)

  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkGeovis vtkRendering vtkIO)
  
  SET (TestsToRun ${Tests})
  REMOVE (TestsToRun ${KIT}CxxTests.cxx)

  #
     # Add all the executables
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(Geovis-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(Geovis-${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test)

  IF (VTK_DATA_ROOT)
    set_tests_properties(Geovis-TestGeoView PROPERTIES RUN_SERIAL ON)
    set_tests_properties(Geovis-TestGeoView2D PROPERTIES RUN_SERIAL ON)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_DISPLAY)
