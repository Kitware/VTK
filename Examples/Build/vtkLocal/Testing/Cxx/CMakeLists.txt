ADD_EXECUTABLE(vtkLocalTest vtkLocalTest.cxx)
TARGET_LINK_LIBRARIES(vtkLocalTest vtkLocal)

ADD_TEST(vtkLocalTest ${EXECUTABLE_OUTPUT_PATH}/vtkLocalTest)
