LINK_LIBRARIES (
vtkCommon 
)

#
# Add all the executables 
#
ADD_EXECUTABLE(ObjectFactory ObjectFactory)
ADD_EXECUTABLE(otherArrays otherArrays)


ADD_TEST(ObjectFactory ${CXX_TEST_PATH}/ObjectFactory)
ADD_TEST(otherArrays ${CXX_TEST_PATH}/otherArrays)
