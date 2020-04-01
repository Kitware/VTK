cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (Step1)

find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersSources
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingOpenGL2
)
include(${VTK_USE_FILE})
add_executable(Cone MACOSX_BUNDLE Cone.cxx)
target_link_libraries(Cone ${VTK_LIBRARIES})
