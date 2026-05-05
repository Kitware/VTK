vtk_add_test_cxx(vtkGUISupportQtWebGPUCxxTests tests
  TestQVTKWebGPUWidget.cxx
  TestQVTKWebGPUWidgetWithDisabledInteractor.cxx
  )

vtk_add_test_cxx(vtkGUISupportQtWebGPUCxxTests tests
  NO_VALID
  TestQVTKWebGPUWidgetHiDPI.cxx
  TestQVTKWebGPUWidgetMultipleRenderers.cxx
  TestQVTKWebGPUWidgetPicking.cxx
  TestQVTKWebGPUWidgetSwapWindows.cxx
  )

find_package("Qt${vtk_qt_major_version}" COMPONENTS Widgets REQUIRED)

set(CMAKE_AUTOMOC 1)

vtk_test_cxx_executable(vtkGUISupportQtWebGPUCxxTests tests)

target_link_libraries(vtkGUISupportQtWebGPUCxxTests
  PRIVATE
    "Qt${vtk_qt_major_version}::Widgets")

target_compile_definitions(vtkGUISupportQtWebGPUCxxTests PRIVATE QT_NO_KEYWORDS)
