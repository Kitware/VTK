include(vtkQt)

set(MocHeaders ${QVTKMocHeaders}
  QVTKGraphicsItem.h
  QVTKWidget2.h
  )
set(LibSrcs ${QVTKLibSrcs}
  QVTKGraphicsItem.cxx
  QVTKWidget2.cxx
  )

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5OpenGL REQUIRED QUIET)
  include_directories(${Qt5OpenGL_INCLUDE_DIRS})
  add_definitions(${Qt5OpenGL_DEFINITIONS})
  qt5_wrap_cpp(LibMocSrcs ${MocHeaders})

  set(QT_LIBRARIES ${Qt5OpenGL_LIBRARIES})

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt5OpenGL)
else()
  # import Qt4 build settings
  set(QT_USE_QTOPENGL 1)
  find_package(Qt4 REQUIRED QUIET)
  include(${QT_USE_FILE})

  qt4_wrap_cpp(LibMocSrcs ${MocHeaders})
endif()

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${LibMocSrcs})
vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
