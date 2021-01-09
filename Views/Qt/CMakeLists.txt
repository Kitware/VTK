set(classes
  vtkQtAnnotationView
  vtkQtListView
  vtkQtRecordView
  vtkQtTableRepresentation
  vtkQtTableView
  vtkQtTreeView
  vtkQtView)

include(vtkQt)
vtk_module_find_package(
  PACKAGE     Qt${VTK_QT_MAJOR_VERSION}
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt${VTK_QT_MAJOR_VERSION}_VERSION)
set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::ViewsQt
  CLASSES ${classes})

vtk_module_definitions(VTK::ViewsQt PRIVATE QT_NO_KEYWORDS)
vtk_module_link(VTK::ViewsQt PRIVATE Qt${VTK_QT_MAJOR_VERSION}::Widgets)
