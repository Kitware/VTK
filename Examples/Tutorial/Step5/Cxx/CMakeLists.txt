cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Step5)

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

add_executable(Cone5 MACOSX_BUNDLE Cone5.cxx)
target_link_libraries(Cone5 ${VTK_LIBRARIES})
