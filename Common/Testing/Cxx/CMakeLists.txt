LINK_LIBRARIES (
vtkCommon 
)

#
# Add all the executables 
#
ADD_EXECUTABLE(ObjectFactory ObjectFactory)
ADD_EXECUTABLE(otherArrays otherArrays)
ADD_EXECUTABLE(otherRectilinearGrid otherRectilinearGrid)
ADD_EXECUTABLE(otherCellBoundaries otherCellBoundaries)
ADD_EXECUTABLE(otherCellDerivatives otherCellDerivatives)
ADD_EXECUTABLE(otherCellPosition otherCellPosition)
ADD_EXECUTABLE(otherStructuredGrid otherStructuredGrid)


ADD_TEST(ObjectFactory ${CXX_TEST_PATH}/ObjectFactory)
ADD_TEST(otherArrays ${CXX_TEST_PATH}/otherArrays)
ADD_TEST(otherRectilinearGrid ${CXX_TEST_PATH}/otherRectilinearGrid)
ADD_TEST(otherCellBoundaries ${CXX_TEST_PATH}/otherCellBoundaries)
ADD_TEST(otherCellDerivatives ${CXX_TEST_PATH}/otherCellDerivatives)
ADD_TEST(otherCellPosition ${CXX_TEST_PATH}/otherCellPosition)
ADD_TEST(otherStructuredGrid ${CXX_TEST_PATH}/otherStructuredGrid)

