cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

project (Step3)

find_package(VTK COMPONENTS
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

add_executable(Cone3 MACOSX_BUNDLE Cone3.cxx)
target_link_libraries(Cone3 ${VTK_LIBRARIES})
