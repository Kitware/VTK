include(vtkQt)

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

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5 COMPONENTS Widgets REQUIRED QUIET)
  include_directories(${Qt5Widgets_INCLUDE_DIRS})
  add_definitions(${Qt5Widgets_DEFINITIONS})

  qt5_wrap_cpp(LibMocSrcs ${MocHeaders})

  set(QT_LIBRARIES ${Qt5Widgets_LIBRARIES})

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt5 COMPONENTS Widgets)
else()
  # import Qt4 build settings
  find_package(Qt4 REQUIRED QtCore QtGui QUIET)
  include_directories(${QT_INCLUDES})
  add_definitions(${QT_DEFINITIONS})

  qt4_wrap_cpp(LibMocSrcs ${MocHeaders})

  set(QT_LIBRARIES Qt4::QtCore Qt4::QtGui)

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt4 COMPONENTS QtCore QtGui)
endif()

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${LibMocSrcs})
target_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
