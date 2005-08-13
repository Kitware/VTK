# if we are using the display add the tests
IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  SET(KIT VolumeRendering)
  # add tests that do not require data
  SET(MyTests)
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests
      HomogeneousRayIntegration.cxx
      LinearRayIntegration.cxx
      PartialPreIntegration.cxx
      PreIntegrationIncremental.cxx
      PreIntegrationNonIncremental.cxx
      TestProjectedTetrahedra.cxx
      volProt.cxx
      )
    SET(MyTestSupport
      ExerciseUnstructuredGridRayCastMapper.cxx
      )
  ENDIF (VTK_DATA_ROOT)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkTestDriver.h)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests} ${MyTestSupport} )
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkVolumeRendering)
  SET (TestsToRun ${Tests})
  REMOVE (TestsToRun ${KIT}CxxTests.cxx)

  #
  # Add all the executables 
  FOREACH (test ${TestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    IF (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
          -D ${VTK_DATA_ROOT}
          -T ${VTK_BINARY_DIR}/Testing/Temporary
          -V Baseline/${KIT}/${TName}.png)
    ELSE (VTK_DATA_ROOT)
      ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test) 
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
