vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestQVTKWidget2.cxx,NO_DATA
  )
if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED QUIET)
  qt5_wrap_cpp(moc_srcs QTestMainWindow.h)
else ()
  find_package(Qt4 REQUIRED)
  set(QT_USE_QTGUI 1)
  set(QT_USE_QTOPENGL 1)
  include(${QT_USE_FILE})
  qt4_wrap_cpp(moc_srcs QTestMainWindow.h)
endif ()
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  QTestMainWindow.cxx
  ${moc_srcs})
target_link_libraries(${vtk-module}CxxTests LINK_PRIVATE
  ${QT_QTGUI_LIBRARY}
  ${QT_QTOPENGL_LIBRARY}
  ${QT_QTWIDGETS_LIBRARY})
