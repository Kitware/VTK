vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  NO_VALID
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  )

vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  -w QVTKOpenGLWidget
  TestQVTKOpenGLWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQVTKOpenGLWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQVTKOpenGLWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQVTKOpenGLWidget,TestQtWidget.cxx
  TestQVTKOpenGLWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQVTKOpenGLWidgetWithMSAA,TestQtWithMSAA.cxx
  )

vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  -w QVTKOpenGLNativeWidget
  TestQVTKOpenGLNativeWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQVTKOpenGLNativeWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQVTKOpenGLNativeWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQVTKOpenGLNativeWidget,TestQtWidget.cxx
  TestQVTKOpenGLNativeWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQVTKOpenGLNativeWidgetWithMSAA,TestQtWithMSAA.cxx
  )

vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  -w QVTKOpenGLWindow
  TestQVTKOpenGLWindowQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQVTKOpenGLWindowPicking,TestQtPicking.cxx,NO_VALID
  TestQVTKOpenGLWindow,TestQtWidget.cxx
  TestQVTKOpenGLWindowWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQVTKOpenGLWindowWithMSAA,TestQtWithMSAA.cxx
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
  QTestApp.cxx
  TestQtCommon.cxx)

target_link_libraries(vtkGUISupportQtCxxTests
  PRIVATE
    Qt5::Widgets)

set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLNativeWidgetPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWindowPicking
 PROPERTIES RUN_SERIAL ON)

# since these tests grab from on-screen buffer, they must be run serially.
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLNativeWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWindowWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
