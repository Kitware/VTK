find_package("Qt${vtk_qt_major_version}"
  COMPONENTS Quick Qml
  REQUIRED
  )

set(CMAKE_AUTORCC ON)

vtk_add_test_cxx(vtkGUISupportQtQuickCxxTests tests
  TestQQuickVTKItem_1.cxx
  TestQQuickVTKItem_2.cxx
  TestQQuickVTKItem_3.cxx
  # VTK_DEPRECATED_IN_9_3_0 applies to these three tests
  TestQQuickVTKRenderItem.cxx
  TestQQuickVTKRenderItemWidget.cxx
  TestQQuickVTKRenderWindow.cxx
  )

# VTK_DEPRECATED_IN_9_3_0 applies here
set(qml_files
  TestQQuickVTKRenderItem.qml
  TestQQuickVTKRenderItemWidget.qml
  TestQQuickVTKRenderWindow.qml)

foreach (qml_file IN LISTS qml_files)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${qml_file}.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${qml_file}"
    @ONLY)
endforeach ()

set(qml_files
  TestQQuickVTKItem_1.qml
  TestQQuickVTKItem_2.qml
  TestQQuickVTKItem_3.qml)

foreach (qml_file IN LISTS qml_files)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${qml_file}"
    "${CMAKE_CURRENT_BINARY_DIR}/${qml_file}"
    COPYONLY)
endforeach ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/TestQQuickVTK.qrc"
  "${CMAKE_CURRENT_BINARY_DIR}/TestQQuickVTK.qrc"
  COPYONLY)

vtk_test_cxx_executable(vtkGUISupportQtQuickCxxTests tests
  "${CMAKE_CURRENT_BINARY_DIR}/TestQQuickVTK.qrc"
  TestQQuickCommon.cxx
  )

target_link_libraries(vtkGUISupportQtQuickCxxTests
  PRIVATE
    "Qt${vtk_qt_major_version}::Quick"
    "Qt${vtk_qt_major_version}::Qml"
    )

target_compile_definitions(vtkGUISupportQtQuickCxxTests PRIVATE QT_NO_KEYWORDS)

# VTK_DEPRECATED_IN_9_3_0 applies here
set_property(
  TEST
    VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItem
    VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderItemWidget
    VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow
  APPEND
  PROPERTY
    ENVIRONMENT "QML2_IMPORT_PATH=${VTK_QML_DIR}")
