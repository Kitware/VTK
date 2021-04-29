find_package("Qt${vtk_qt_major_version}"
  COMPONENTS Quick Qml
  REQUIRED
  )

set(CMAKE_AUTORCC ON)

vtk_add_test_cxx(vtkGUISupportQtQuickCxxTests tests
  TestQQuickVTKRenderItem.cxx
  TestQQuickVTKRenderItemWidget.cxx
  TestQQuickVTKRenderWindow.cxx
  )

vtk_test_cxx_executable(vtkGUISupportQtQuickCxxTests tests
  TestQQuickVTK.qrc
  )

target_link_libraries(vtkGUISupportQtQuickCxxTests
  PRIVATE
    "Qt${vtk_qt_major_version}::Quick"
    "Qt${vtk_qt_major_version}::Qml"
    )

set_property(
  TEST
    VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItem
    VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget
    VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow
  APPEND
  PROPERTY
    ENVIRONMENT "QML2_IMPORT_PATH=${VTK_QML_DIR}")
