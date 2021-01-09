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
  PACKAGE     Qt${VTK_QT_MAJOR_VERSION}
  COMPONENTS  Widgets Sql
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt${VTK_QT_MAJOR_VERSION}_VERSION)
vtk_module_link(VTK::GUISupportQtSQL
  PUBLIC Qt${VTK_QT_MAJOR_VERSION}::Sql Qt${VTK_QT_MAJOR_VERSION}::Widgets)
