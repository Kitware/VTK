IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)
  SET(KIT Imaging)
  CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
    ImportExport.cxx
    EXTRA_INCLUDE vtkTestDriver.h
    )
  ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO)
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
    ENDIF (VTK_DATA_ROOT)
  ENDFOREACH (test) 
ENDIF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

