LINK_LIBRARIES (
vtkCommon 
vtkFiltering
)

#
# Add all the executables 
#
ADD_EXECUTABLE(otherColorTransferFunction otherColorTransferFunction.cxx)

ADD_TEST(otherColorTransferFunction ${CXX_TEST_PATH}/otherColorTransferFunction)
