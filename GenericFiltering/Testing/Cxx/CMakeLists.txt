# The bridge serves two purposes:
# 1. Show the users how to implement a vtkGenericDataSet
# 2. Test and validate the filters on a vtkGenericDataset on standard VTK
# classes.

# C++ tests for the bridge between standard VTK classes and the
# vtkGenericDataSet.

INCLUDE_DIRECTORIES(
  ${VTK_SOURCE_DIR}/GenericFiltering/Testing/Cxx/Bridge
  )

SET(KIT Bridge)
SET(UKIT BRIDGE)
SET( Kit_SRCS
vtkBridgeCell.cxx
vtkBridgeDataSet.cxx
vtkBridgeAttribute.cxx
vtkBridgeCellIterator.cxx
vtkBridgePointIterator.cxx
vtkBridgeCellIteratorStrategy.cxx
vtkBridgeCellIteratorOnDataSet.cxx
vtkBridgeCellIteratorOne.cxx
vtkBridgeCellIteratorOnCellBoundaries.cxx
vtkBridgeCellIteratorOnCellList.cxx
)

SET_SOURCE_FILES_PROPERTIES(
vtkBridgeCell.cxx
vtkBridgeCellIterator.cxx
vtkBridgePointIterator.cxx
vtkBridgeAttribute.cxx
vtkBridgeCellIteratorStrategy.cxx
vtkBridgeCellIteratorOnDataSet.cxx
vtkBridgeCellIteratorOne.cxx
vtkBridgeCellIteratorOnCellBoundaries.cxx
vtkBridgeCellIteratorOnCellList.cxx
  WRAP_EXCLUDE
)

#
# Create a test lists
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  otherCreation.cxx
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests} ${Kit_SRCS} )
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkFiltering)

SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
ENDFOREACH (test)
