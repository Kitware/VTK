SET(KIT Rendering)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  otherCoordinate.cxx
  EXTRA_INCLUDE vtkSuppressCrtDbgWarning.h
  FUNCTION vtkSuppressCrtDbgWarning
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO)


SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables 
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}) 
ENDFOREACH (test) 

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







