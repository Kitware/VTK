#
# Add all the executables that require rendering
#
FOREACH (exe
  TimeRenderer
  TimeRenderer2
  otherCoordinate
  TestRenderWindow
  )

  ADD_EXECUTABLE(${exe} ${exe}.cxx)
  TARGET_LINK_LIBRARIES(${exe} vtkRendering vtkIO)
ENDFOREACH (exe)

ADD_TEST(otherCoordinate ${CXX_TEST_PATH}/otherCoordinate)

ADD_TEST(TestRenderWindow-image ${CXX_TEST_PATH}/TestRenderWindow
                      -D ${VTK_DATA_ROOT}
                      -FrontBuffer
                      -V Baseline/Rendering/TestRenderWindow.png)






