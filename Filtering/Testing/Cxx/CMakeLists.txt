LINK_LIBRARIES (
vtkCommon 
vtkFiltering
)

#
# Add all the executables 
#
ADD_EXECUTABLE(otherColorTransferFunction otherColorTransferFunction)

ADD_TEST(otherColorTransferFunction ${CXX_TEST_PATH}/otherColorTransferFunction)
