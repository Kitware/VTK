SET(KIT Filtering)
#
# Create a test lists
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  otherCellArray.cxx
  otherCellBoundaries.cxx
  otherCellPosition.cxx
  otherCellTypes.cxx
  otherColorTransferFunction.cxx
  otherEmptyCell.cxx
  otherFieldData.cxx
  otherRectilinearGrid.cxx
  otherStructuredGrid.cxx
  quadCellConsistency.cxx
  quadraticEvaluation.cxx
  TestAMRBox.cxx
  TestInterpolationFunctions.cxx
  TestInterpolationDerivs.cxx
  TestImageDataFindCell.cxx
  TestImageIterator.cxx
  TestGenericCell.cxx
  TestGraph.cxx
  TestHigherOrderCell.cxx  
  TestPointLocators.cxx
  TestPolyDataRemoveCell.cxx  
  TestPolygon.cxx
  TestTreeBFSIterator.cxx
  TestTriangle.cxx

  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkFiltering vtkGraphics)

SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
ENDFOREACH (test)

