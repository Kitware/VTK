SET(KIT Filtering)
# Add tests that require data
IF (VTK_DATA_ROOT)
  SET(DataBasedTests
      TestLinearSelector2D.cxx
      TestLinearSelector3D.cxx
      )
ENDIF (VTK_DATA_ROOT)
#
# Create a test lists
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  CellTreeLocator.cxx
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
  TestGraphWeightEuclideanDistanceFilter.cxx
  TestHigherOrderCell.cxx
  TestPointLocators.cxx
  TestPolyDataRemoveCell.cxx
  TestPolygon.cxx
  TestSelectionSubtract.cxx
  TestTreeBFSIterator.cxx
  TestTreeDFSIterator.cxx
  TestTriangle.cxx
  TestImageDataInterpolation.cxx
  TestImageDataToStructuredGrid.cxx
  ${DataBasedTests}
  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkIO vtkFiltering vtkGraphics)

SET (TestsToRun ${Tests})
LIST (REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

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
    ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName}
      -T ${VTK_BINARY_DIR}/Testing/Temporary
      )
  ENDIF (VTK_DATA_ROOT)
ENDFOREACH (test)

