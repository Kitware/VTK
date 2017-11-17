include(vtkQt)

set(MocHeaders ${QVTKMocHeaders}
  QVTKGraphicsItem.h
  )
set(LibSrcs ${QVTKLibSrcs}
  QVTKGraphicsItem.cxx
  )

if(NOT VTK_LEGACY_REMOVE)
  list(APPEND MocHeaders QVTKWidget2.h)
  list(APPEND LibSrcs QVTKWidget2.cxx)
endif()

add_definitions(-DVTK_OPENGL2)

find_package(Qt5 COMPONENTS OpenGL REQUIRED QUIET)
include_directories(${Qt5OpenGL_INCLUDE_DIRS})
add_definitions(${Qt5OpenGL_DEFINITIONS})
qt5_wrap_cpp(LibMocSrcs ${MocHeaders})

set(QT_LIBRARIES ${Qt5OpenGL_LIBRARIES})

# When this module is loaded by an app, load Qt too.
vtk_module_export_code_find_package(Qt5 COMPONENTS OpenGL)

set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${LibMocSrcs})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${QT_LIBRARIES})
