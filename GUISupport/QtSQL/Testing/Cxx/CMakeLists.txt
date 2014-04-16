include(vtkQt)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5Widgets REQUIRED)
  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  add_definitions(${Qt5Widgets_DEFINITIONS})
else()
  find_package(Qt4 REQUIRED)
  include(${QT_USE_FILE})
endif()

vtk_test_cxx_executable(${vtk-module}CxxTests tests)

if(VTK_QT_VERSION VERSION_GREATER "4")
  set_target_properties(${vtk-module}CxxTests PROPERTIES
    COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
endif()
