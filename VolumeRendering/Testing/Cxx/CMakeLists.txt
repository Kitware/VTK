# if we are using the display add the tests
IF (VTK_USE_DISPLAY)
  SET(KIT VolumeRendering)
  IF (VTK_DATA_ROOT)
    # add tests that require data
    SET(MyTests 
      volProt.cxx
      )
  ENDIF (VTK_DATA_ROOT)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                         EXTRA_INCLUDE vtkSuppressCrtDbgWarning.h
                         FUNCTION vtkSuppressCrtDbgWarning)
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests} )
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
ENDIF (VTK_USE_DISPLAY)
