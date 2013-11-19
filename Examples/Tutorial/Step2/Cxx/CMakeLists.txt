cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

project (Step2)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

add_executable(Cone2 MACOSX_BUNDLE Cone2.cxx)
target_link_libraries(Cone2 ${VTK_LIBRARIES})
