# The bridge serves two purposes:
# 1. Show the users how to implement a vtkGenericDataSet
# 2. Test and validate the filters on a vtkGenericDataset on standard VTK
# classes.

# C++ tests for the bridge between standard VTK classes and the
# vtkGenericDataSet.

IF (VTK_USE_RENDERING AND VTK_USE_DISPLAY)

INCLUDE_DIRECTORIES(
  ${VTK_SOURCE_DIR}/GenericFiltering/Testing/Cxx/Bridge
  )

#SET(KIT Bridge)
SET(KIT GenericFiltering)
SET(UKIT BRIDGE)
SET( Kit_SRCS
vtkBridgeCell.cxx
vtkBridgeDataSet.cxx
vtkBridgeAttribute.cxx
vtkBridgeCellIterator.cxx
vtkBridgePointIterator.cxx
vtkBridgePointIteratorOnCell.cxx
vtkBridgePointIteratorOnDataSet.cxx
vtkBridgePointIteratorOne.cxx
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
vtkBridgePointIteratorOnCell.cxx
vtkBridgePointIteratorOnDataSet.cxx
vtkBridgePointIteratorOne.cxx
vtkBridgeAttribute.cxx
vtkBridgeCellIteratorStrategy.cxx
vtkBridgeCellIteratorOnDataSet.cxx
vtkBridgeCellIteratorOne.cxx
vtkBridgeCellIteratorOnCellBoundaries.cxx
vtkBridgeCellIteratorOnCellList.cxx
  WRAP_EXCLUDE
)

# add tests that do not require data
SET(MyTests
 otherCreation.cxx
)

# add tests that require data
IF (VTK_DATA_ROOT)
 SET(MyTests ${MyTests}
  TestGenericDataSetTessellator.cxx
  TestGenericGeometryFilter.cxx
  TestGenericGlyph3DFilter.cxx
  TestGenericContourFilter.cxx
  TestGenericClip.cxx
  TestGenericCutter.cxx
  TestGenericStreamTracer.cxx
  TestSmoothErrorMetric.cxx
  TestViewDependentErrorMetric.cxx
 )
ENDIF (VTK_DATA_ROOT)

#
# Create a test lists
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests} ${Kit_SRCS} )
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkGenericFiltering vtkRendering vtkIO)

SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

#
# Add all the executables
#FOREACH (test ${TestsToRun})
#  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
#  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
#ENDFOREACH (test)

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
