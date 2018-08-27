include(vtkQt)

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
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

vtk_test_cxx_executable(vtkGUISupportQtCxxTests tests
  QTestApp.cxx)

set_target_properties(vtkGUISupportQtCxxTests PROPERTIES
  COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

set_tests_properties(vtkGUISupportQtCxx-TestQVTKOpenGLWidgetPicking
 PROPERTIES RUN_SERIAL ON)
