IF (VTK_USE_RENDERING)
  #
  # Add all the executables that require rendering
  #
  ADD_EXECUTABLE(BoxWidget BoxWidget.cxx)
  ADD_EXECUTABLE(ImagePlaneWidget ImagePlaneWidget.cxx)
  ADD_EXECUTABLE(AmoebaMinimizer AmoebaMinimizer.cxx)

  TARGET_LINK_LIBRARIES(BoxWidget vtkHybrid)
  TARGET_LINK_LIBRARIES(ImagePlaneWidget vtkHybrid)
  TARGET_LINK_LIBRARIES(AmoebaMinimizer vtkHybrid)

  #
  # Add tests, with the data
  #
  IF (VTK_DATA_ROOT)
    ADD_TEST(BoxWidget-image ${CXX_TEST_PATH}/BoxWidget
                -D ${VTK_DATA_ROOT}
                -V Baseline/Hybrid/BoxWidget.png)
    ADD_TEST(ImagePlaneWidget-image ${CXX_TEST_PATH}/ImagePlaneWidget
                -D ${VTK_DATA_ROOT}
                -V Baseline/Hybrid/ImagePlaneWidget.png)
    ADD_TEST(AmoebaMinimizer ${CXX_TEST_PATH}/AmoebaMinimizer)
  ELSE (VTK_DATA_ROOT)
    ADD_TEST(BoxWidget ${CXX_TEST_PATH}/BoxWidget)
    ADD_TEST(ImagePlaneWidget ${CXX_TEST_PATH}/ImagePlaneWidget)
    ADD_TEST(AmoebaMinimizer ${CXX_TEST_PATH}/AmoebaMinimizer)
  ENDIF (VTK_DATA_ROOT)

ENDIF (VTK_USE_RENDERING)
