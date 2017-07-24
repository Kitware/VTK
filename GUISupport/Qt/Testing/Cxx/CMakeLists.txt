include(vtkQt)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  )

if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    TestQVTKOpenGLWidget.cxx
    TestQVTKOpenGLWidgetWithDisabledInteractor.cxx
    TestQVTKOpenGLWidgetWithMSAA.cxx
    )
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    NO_VALID
    TestQVTKOpenGLWidgetPicking.cxx
    )
endif()

if(NOT VTK_LEGACY_REMOVE)
  # tests use legacy QVTKWidget.
  if(WIN32)
    vtk_add_test_cxx(${vtk-module}CxxTests tests
      NO_VALID
      TestWin32QVTKWidget.cxx)
  endif()
  vtk_add_test_cxx(${vtk-module}CxxTests tests
    NO_VALID
    TestQtSwapWindows.cxx)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
vtk_test_cxx_executable(${vtk-module}CxxTests tests QTestApp.cxx)
target_link_libraries(${vtk-module}CxxTests LINK_PRIVATE Qt5::Widgets)
