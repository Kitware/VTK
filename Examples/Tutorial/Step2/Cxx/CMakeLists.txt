cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


project (Step2)
find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersSources
  vtkFiltersSources
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
)
include(${VTK_USE_FILE})

add_executable(Cone2 MACOSX_BUNDLE Cone2.cxx)
target_link_libraries(Cone2 ${VTK_LIBRARIES})