include(vtkQt)

vtk_add_test_cxx(vtkGUISupportQtSQLCxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

find_package(Qt5 COMPONENTS Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

vtk_test_cxx_executable(vtkGUISupportQtSQLCxxTests tests)

set_target_properties(vtkGUISupportQtSQLCxxTests PROPERTIES
  COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
