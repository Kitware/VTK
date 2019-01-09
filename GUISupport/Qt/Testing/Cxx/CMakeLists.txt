vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  NO_VALID
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  TestQVTKOpenGLNativeWidgetPicking.cxx
  TestQVTKOpenGLWidgetPicking.cxx
  TestQVTKOpenGLNativeWidgetSwapWindows.cxx
  TestQVTKOpenGLWidgetSwapWindows.cxx
  )

vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  TestQVTKOpenGLNativeWidget.cxx
  TestQVTKOpenGLWidget.cxx
  TestQVTKOpenGLNativeWidgetWithDisabledInteractor.cxx
  TestQVTKOpenGLNativeWidgetWithMSAA.cxx
  TestQVTKOpenGLWidgetWithMSAA.cxx
  )

if(NOT VTK_LEGACY_REMOVE)
  # tests use legacy QVTKWidget.
  if(WIN32)
    vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
      NO_VALID
      TestWin32QVTKWidget.cxx)
  endif()
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

vtk_test_cxx_executable(vtkGUISupportQtCxxTests tests
  QTestApp.cxx)

target_link_libraries(vtkGUISupportQtCxxTests
  PRIVATE
    Qt5::Widgets)

set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetPicking
 PROPERTIES RUN_SERIAL ON)
