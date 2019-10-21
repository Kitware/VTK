set(classes
  vtkQtAnnotationView
  vtkQtListView
  vtkQtRecordView
  vtkQtTableRepresentation
  vtkQtTableView
  vtkQtTreeView
  vtkQtView)

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::ViewsQt
  CLASSES ${classes})

vtk_module_link(VTK::ViewsQt PRIVATE Qt5::Widgets)
