LINK_LIBRARIES (
vtkCommon 
)

#
# Add all the executables 
#
ADD_EXECUTABLE(ObjectFactory ObjectFactory)


ADD_TEST(ObjectFactory ${CXX_TEST_PATH}/ObjectFactory)



