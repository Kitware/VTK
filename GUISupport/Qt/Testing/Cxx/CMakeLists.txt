vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  NO_VALID
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  )

vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  -w QVTKOpenGLStereoWidget
  TestQVTKOpenGLStereoWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQVTKOpenGLStereoWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQVTKOpenGLStereoWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQVTKOpenGLStereoWidget,TestQtWidget.cxx
  TestQVTKOpenGLStereoWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQVTKOpenGLStereoWidgetWithMSAA,TestQtWithMSAA.cxx
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

vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  -w QVTKRenderWidget
  TestQVTKRenderWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQVTKRenderWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQVTKRenderWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQVTKRenderWidget,TestQtWidget.cxx
  TestQVTKRenderWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQVTKRenderWidgetWithMSAA,TestQtWithMSAA.cxx
  )

if (NOT VTK_LEGACY_REMOVE)
vtk_add_test_cxx(vtkGUISupportQtCxxTests tests
  -w QVTKOpenGLWidget
  TestQVTKOpenGLWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQVTKOpenGLWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQVTKOpenGLWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQVTKOpenGLWidget,TestQtWidget.cxx
  TestQVTKOpenGLWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQVTKOpenGLWidgetWithMSAA,TestQtWithMSAA.cxx
  )
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

vtk_test_cxx_executable(vtkGUISupportQtCxxTests tests
  QTestApp.cxx
  TestQtCommon.cxx)

target_link_libraries(vtkGUISupportQtCxxTests
  PRIVATE
    Qt5::Widgets)

set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLNativeWidgetPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWindowPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKRenderWidgetPicking
 PROPERTIES RUN_SERIAL ON)

if (NOT VTK_LEGACY_REMOVE)
  set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetPicking
   PROPERTIES RUN_SERIAL ON)
endif()


# since these tests grab from on-screen buffer, they must be run serially.
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLStereoWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLNativeWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWindowWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(VTK::GUISupportQtCxx-TestQVTKRenderWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)

if (NOT VTK_LEGACY_REMOVE)
  set_tests_properties(VTK::GUISupportQtCxx-TestQVTKOpenGLWidgetWithDisabledInteractor
   PROPERTIES RUN_SERIAL ON)
endif()
