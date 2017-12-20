if(NOT VTK_LEGACY_REMOVE)
vtk_add_test_cxx(vtkGUISupportQtOpenGLCxxTests tests
  TestQVTKWidget2.cxx,NO_DATA
  )
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED QUIET)
qt5_wrap_cpp(moc_srcs QTestMainWindow.h)
vtk_test_cxx_executable(vtkGUISupportQtOpenGLCxxTests tests
  QTestMainWindow.cxx
  ${moc_srcs})
target_link_libraries(vtkGUISupportQtOpenGLCxxTests LINK_PRIVATE
  ${QT_QTGUI_LIBRARY}
  ${QT_QTOPENGL_LIBRARY}
  ${QT_QTWIDGETS_LIBRARY})
endif()
