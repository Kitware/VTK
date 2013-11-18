cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Step6)

find_package(VTK COMPONENTS
  vtkFiltersSources
  vtkInteractionWidgets
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

add_executable(Cone6 MACOSX_BUNDLE Cone6.cxx)
find_package( VTK )
target_link_libraries(Cone6 ${VTK_LIBRARIES})
