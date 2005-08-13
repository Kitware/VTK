SET(KIT Filtering)
#
# Create a test lists
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  quadCellConsistency.cxx
  otherColorTransferFunction.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkFiltering)

SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables 
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
ENDFOREACH (test) 

