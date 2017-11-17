include(vtkQt)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  TestQVTKOpenGLWidgetPicking.cxx
  TestQtSwapWindows.cxx
  )

vtk_add_test_cxx(${vtk-module}CxxTests tests
  TestQVTKOpenGLWidget.cxx
  TestQVTKOpenGLWidgetWithDisabledInteractor.cxx
  TestQVTKOpenGLWidgetWithMSAA.cxx
  )

if(NOT VTK_LEGACY_REMOVE)
  # tests use legacy QVTKWidget.
  if(WIN32)
    vtk_add_test_cxx(${vtk-module}CxxTests tests
      NO_VALID
      TestWin32QVTKWidget.cxx)
  endif()
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  QTestApp.cxx)

set_target_properties(${vtk-module}CxxTests PROPERTIES
  COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
