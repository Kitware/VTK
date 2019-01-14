set(LibSrcs
  vtkQtAnnotationView.cxx
  vtkQtListView.cxx
  vtkQtRecordView.cxx
  vtkQtTableRepresentation.cxx
  vtkQtTableView.cxx
  vtkQtTreeView.cxx
  vtkQtView.cxx
  )
set(MocHeaders
  vtkQtAnnotationView.h
  vtkQtListView.h
  vtkQtRecordView.h
  vtkQtTableView.h
  vtkQtTreeView.h
  vtkQtView.h
  )

vtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
qt5_wrap_cpp(LibMocSrcs ${MocHeaders})

vtk_module_add_module(VTK::ViewsQt
  SOURCES ${LibSrcs} ${LibMocSrcs}
  HEADERS ${MocHeaders})

vtk_module_link(VTK::ViewsQt PRIVATE Qt5::Widgets)
