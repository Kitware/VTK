vtk_add_test_cxx(vtkGUISupportQtSQLCxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

find_package("Qt${vtk_qt_major_version}" COMPONENTS Widgets REQUIRED)

vtk_test_cxx_executable(vtkGUISupportQtSQLCxxTests tests)
target_link_libraries(vtkGUISupportQtSQLCxxTests
  PRIVATE
    "Qt${vtk_qt_major_version}::Widgets")
