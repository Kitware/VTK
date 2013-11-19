cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

project (Step1)

find_package(VTK COMPONENTS
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

add_executable(Cone MACOSX_BUNDLE Cone.cxx)
target_link_libraries(Cone ${VTK_LIBRARIES})
