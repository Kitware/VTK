include(vtkQt)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

find_package(Qt5 COMPONENTS Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
add_definitions(${Qt5Widgets_DEFINITIONS})

vtk_test_cxx_executable(${vtk-module}CxxTests tests)

set_target_properties(${vtk-module}CxxTests PROPERTIES
  COMPILE_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
