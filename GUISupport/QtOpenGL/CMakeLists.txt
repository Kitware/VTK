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

if (VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  add_definitions(-DVTK_OPENGL2)
endif ()

find_package(Qt5 COMPONENTS OpenGL REQUIRED QUIET)

# When this module is loaded by an app, load Qt too.
vtk_module_export_code_find_package(Qt5 COMPONENTS OpenGL)

qt5_wrap_cpp(LibMocSrcs ${MocHeaders})
set(${vtk-module}_NO_HeaderTest 1)
vtk_module_library(${vtk-module} ${LibSrcs} ${LibMocSrcs})
vtk_module_link_libraries(${vtk-module} LINK_PRIVATE Qt5::OpenGL)

include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
