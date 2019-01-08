# set up sources to build
set(classes
  vtkQtSQLDatabase
  vtkQtSQLQuery
  vtkQtTimePointUtility
  )

vtk_module_add_module(VTK::GUISupportQtSQL
  CLASSES ${classes})

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Widgets Sql
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
vtk_module_link(VTK::GUISupportQtSQL PUBLIC Qt5::Sql Qt5::Widgets)
