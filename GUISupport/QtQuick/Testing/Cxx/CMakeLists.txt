find_package ("Qt${vtk_qt_major_version}"
  COMPONENTS Quick Qml
  REQUIRED
  )

set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTORCC ON)

vtk_add_test_cxx (vtkGUISupportQtQuickCxxTests tests
  TestQQuickVTKRenderWindow.cxx
  )

vtk_test_cxx_executable(vtkGUISupportQtQuickCxxTests tests
  TestQQuickVTKRenderWindow.qrc
  )

target_link_libraries(vtkGUISupportQtQuickCxxTests
  PRIVATE
    "Qt${vtk_qt_major_version}::Quick"
    "Qt${vtk_qt_major_version}::Qml"
    )

set_tests_properties(VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow
  PROPERTIES ENVIRONMENT "QML2_IMPORT_PATH=${CMAKE_BINARY_DIR}/$<CONFIG>")
