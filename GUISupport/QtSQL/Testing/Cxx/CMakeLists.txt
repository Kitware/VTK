include(vtkQt)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

find_package(Qt5 COMPONENTS Widgets REQUIRED)
vtk_test_cxx_executable(${vtk-module}CxxTests tests)
target_link_libraries(${vtk-module}CxxTests LINK_PRIVATE Qt5::Widgets)
