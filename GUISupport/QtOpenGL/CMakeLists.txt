include(vtkQt)

set(MocHeaders ${QVTKMocHeaders}
  QVTKGraphicsItem.h
  QVTKWidget2.h
  )
set(LibSrcs ${QVTKLibSrcs}
  QVTKGraphicsItem.cxx
  QVTKWidget2.cxx
  )

if (VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  add_definitions(-DVTK_OPENGL2)
endif ()

if(VTK_QT_VERSION VERSION_GREATER "4")
  find_package(Qt5 COMPONENTS OpenGL REQUIRED QUIET)
  include_directories(${Qt5OpenGL_INCLUDE_DIRS})
  add_definitions(${Qt5OpenGL_DEFINITIONS})
  qt5_wrap_cpp(LibMocSrcs ${MocHeaders})

  set(QT_LIBRARIES ${Qt5OpenGL_LIBRARIES})

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt5 COMPONENTS OpenGL)
else()
  # import Qt4 build settings
  find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED QUIET)
  include_directories(${QT_INCLUDES})
  add_definitions(${QT_DEFINITIONS})

  set(QT_LIBRARIES Qt4::QtCore Qt4::QtGui Qt4::QtOpenGL)

  qt4_wrap_cpp(LibMocSrcs ${MocHeaders})

  # When this module is loaded by an app, load Qt too.
  vtk_module_export_code_find_package(Qt4 COMPONENTS QtCore QtGui QtOpenGL)
endif()

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${LibMocSrcs})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
