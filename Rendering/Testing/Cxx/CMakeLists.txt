SET(KIT Rendering)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  otherCoordinate.cxx
  TestRenderWindow.cxx
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO)


SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx TestRenderWindow.cxx)

#
# Add all the executables 
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
ENDFOREACH (test) 

# handle TestRenderWindow seperately since it has different arguments
IF (VTK_DATA_ROOT)
  ADD_TEST(TestRenderWindow ${CXX_TEST_PATH}/${KIT}CxxTests TestRenderWindow
                        -D ${VTK_DATA_ROOT}
                        -FrontBuffer
                        -V Baseline/Rendering/TestRenderWindow.png)
ENDIF (VTK_DATA_ROOT)

#
# Add other odd tests or executables
#
FOREACH (exe
  TimeRenderer
  TimeRenderer2
  )
  ADD_EXECUTABLE(${exe} ${exe}.cxx)
  TARGET_LINK_LIBRARIES(${exe} vtkRendering vtkIO)
  IF (APPLE)
    TARGET_LINK_LIBRARIES(${exe} "-framework GLUT")
  ENDIF (APPLE)
ENDFOREACH (exe)







