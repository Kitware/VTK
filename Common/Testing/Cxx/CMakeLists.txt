LINK_LIBRARIES (
vtkCommon 
)

#
# Add all the executables 
#
ADD_EXECUTABLE(ObjectFactory ObjectFactory)
ADD_EXECUTABLE(otherArrays otherArrays)
ADD_EXECUTABLE(otherEmptyCell otherEmptyCell)
ADD_EXECUTABLE(otherRectilinearGrid otherRectilinearGrid)
ADD_EXECUTABLE(otherCellBoundaries otherCellBoundaries)
ADD_EXECUTABLE(otherCellDerivatives otherCellDerivatives)
ADD_EXECUTABLE(otherCellPosition otherCellPosition)
ADD_EXECUTABLE(otherStructuredGrid otherStructuredGrid)
ADD_EXECUTABLE(otherTimerLog otherTimerLog)
ADD_EXECUTABLE(otherLookupTable otherLookupTable)
ADD_EXECUTABLE(otherByteSwap otherByteSwap)
ADD_EXECUTABLE(otherCellArray otherCellArray)

ADD_TEST(ObjectFactory ${CXX_TEST_PATH}/ObjectFactory)
ADD_TEST(otherArrays ${CXX_TEST_PATH}/otherArrays)
ADD_TEST(otherEmptyCell ${CXX_TEST_PATH}/otherEmptyCell)
ADD_TEST(otherRectilinearGrid ${CXX_TEST_PATH}/otherRectilinearGrid)
ADD_TEST(otherCellBoundaries ${CXX_TEST_PATH}/otherCellBoundaries)
ADD_TEST(otherCellDerivatives ${CXX_TEST_PATH}/otherCellDerivatives)
ADD_TEST(otherCellPosition ${CXX_TEST_PATH}/otherCellPosition)
ADD_TEST(otherStructuredGrid ${CXX_TEST_PATH}/otherStructuredGrid)
ADD_TEST(otherTimerLog ${CXX_TEST_PATH}/otherTimerLog)
ADD_TEST(otherLookupTable ${CXX_TEST_PATH}/otherLookupTable)
ADD_TEST(otherByteSwap ${CXX_TEST_PATH}/otherByteSwap)
ADD_TEST(otherCellArray ${CXX_TEST_PATH}/otherCellArray)

