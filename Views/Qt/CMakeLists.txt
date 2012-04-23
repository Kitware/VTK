set(LibSrcs
  vtkQtAnnotationView.cxx
  vtkQtListView.cxx
  vtkQtRecordView.cxx
  vtkQtTableRepresentation.cxx
  vtkQtTableView.cxx
  vtkQtTreeView.cxx
  vtkQtView.cxx
  )
set_source_files_properties(vtkQtView.cxx ABSTRACT)
set(MocHeaders
  vtkQtAnnotationView.h
  vtkQtListView.h
  vtkQtRecordView.h
  vtkQtTableView.h
  vtkQtTreeView.h
  vtkQtView.h
  )

# import Qt4 build settings
set(QT_USE_QTNETWORK 1)
find_package(Qt4 REQUIRED QUIET)
include(${QT_USE_FILE})

qt4_wrap_cpp(LibMocSrcs ${MocHeaders})

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${LibMocSrcs})

target_link_libraries(${vtk-module} ${QT_LIBRARIES})
