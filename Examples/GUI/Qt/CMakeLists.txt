if(NOT VTK_LEGACY_REMOVE)
  # This example uses a 3DConnexion 3D mouse and Qt4 style widget.
  add_subdirectory(Events)
endif()

if(VTK_QT_VERSION VERSION_GREATER "4")
  add_subdirectory(FourPaneViewer)
  add_subdirectory(ImageViewer)
  add_subdirectory(SimpleView)
endif()

if(TARGET vtkGUISupportQtOpenGL)
  if(VTK_QT_VERSION VERSION_GREATER "4")
    find_package(Qt5 COMPONENTS WebKitWidgets QUIET)
    if(Qt5WebKitWidgets_FOUND)
      add_subdirectory(GraphicsView)
    endif()
  else()
    add_subdirectory(GraphicsView)
  endif()
endif()

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/CTestCustom.ctest.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.ctest" @ONLY)
