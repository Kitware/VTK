#
# Add all the executables 
#
ADD_EXECUTABLE(otherColorTransferFunction otherColorTransferFunction.cxx)
TARGET_LINK_LIBRARIES (otherColorTransferFunction vtkFiltering )

ADD_TEST(otherColorTransferFunction
         ${CXX_TEST_PATH}/otherColorTransferFunction)
