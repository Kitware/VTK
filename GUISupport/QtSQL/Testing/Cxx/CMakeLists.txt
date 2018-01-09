include(../../../../CMake/vtkQt.cmake)

vtk_add_test_cxx(vtkGUISupportQtSQLCxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)

vtk_test_cxx_executable(vtkGUISupportQtSQLCxxTests tests)
target_link_libraries(vtkGUISupportQtSQLCxxTests
  PRIVATE
    Qt5::Widgets)
