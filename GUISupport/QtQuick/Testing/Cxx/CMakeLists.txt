find_package ("Qt${vtk_qt_major_version}"
  COMPONENTS Quick Qml
  REQUIRED
  )

set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTOUIC ON)

if (vtk_qt_major_version VERSION_LESS 6)
  qt5_add_resources(QRC_BUILT_SRCS TestQQuickVTKRenderWindow.qrc)
else ()
  qt_add_resources(QRC_BUILT_SRCS TestQQuickVTKRenderWindow.qrc)
endif ()

vtk_add_test_cxx (vtkGUISupportQtQuickCxxTests tests
  TestQQuickVTKRenderWindow.cxx
  )

vtk_test_cxx_executable(vtkGUISupportQtQuickCxxTests tests
  ${QRC_BUILT_SRCS}
  )

target_link_libraries(vtkGUISupportQtQuickCxxTests
  PRIVATE
    "Qt${vtk_qt_major_version}::Quick"
    "Qt${vtk_qt_major_version}::Qml"
    )
  #
  #set_tests_properties(VTK::GUISupportQtQuickCxx-TestQQuickVTKRenderWindow
  #  PROPERTIES ENVIRONMENT "QML2_IMPORT_PATH=C:/Projects/Qt/install/bin/;")
