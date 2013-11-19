add_subdirectory(Events)
add_subdirectory(FourPaneViewer)
add_subdirectory(ImageViewer)
add_subdirectory(SimpleView)

if(TARGET vtkGUISupportQtOpenGL)
  add_subdirectory(GraphicsView)
endif()

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY)
