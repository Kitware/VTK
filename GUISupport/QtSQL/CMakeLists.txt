# set up sources to build
set(classes
  vtkQtSQLDatabase
  vtkQtSQLQuery
  vtkQtTimePointUtility
  )

vtk_module_add_module(VTK::GUISupportQtSQL
  CLASSES ${classes})

include(vtkQt)
vtk_module_find_package(
  PACKAGE     "Qt${vtk_qt_major_version}"
  COMPONENTS  Widgets Sql
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${vtk_qt_major_version}_VERSION")
vtk_module_link(VTK::GUISupportQtSQL
  PUBLIC "Qt${vtk_qt_major_version}::Sql" "Qt${vtk_qt_major_version}::Widgets")
vtk_add_test_mangling(VTK::GUISupportQtSQL)
