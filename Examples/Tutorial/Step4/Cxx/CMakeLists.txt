cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


project (Step4)

find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
)
include(${VTK_USE_FILE})

add_executable(Cone4 MACOSX_BUNDLE Cone4.cxx)
target_link_libraries(Cone4 ${VTK_LIBRARIES})
