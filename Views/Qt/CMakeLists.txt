set(classes
  vtkQtAnnotationView
  vtkQtListView
  vtkQtRecordView
  vtkQtTableRepresentation
  vtkQtTableView
  vtkQtTreeView
  vtkQtView)

include(vtkQt)
vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE     "Qt${vtk_qt_major_version}"
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${vtk_qt_major_version}_VERSION")
set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::ViewsQt
  CLASSES ${classes})

vtk_module_definitions(VTK::ViewsQt PRIVATE QT_NO_KEYWORDS)
vtk_module_link(VTK::ViewsQt PRIVATE "Qt${vtk_qt_major_version}::Widgets")
vtk_add_test_mangling(VTK::ViewsQt)
