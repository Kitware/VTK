vtk_add_test_cxx(vtkGUISupportQtSQLCxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

find_package(Qt${VTK_QT_MAJOR_VERSION} COMPONENTS Widgets REQUIRED)

vtk_test_cxx_executable(vtkGUISupportQtSQLCxxTests tests)
target_link_libraries(vtkGUISupportQtSQLCxxTests
  PRIVATE
    Qt${VTK_QT_MAJOR_VERSION}::Widgets)
