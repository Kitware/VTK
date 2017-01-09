cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


PROJECT (Step6)

find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkInteractionWidgets
  vtkRendering${VTK_RENDERING_BACKEND}
)
include(${VTK_USE_FILE})

add_executable(Cone6 MACOSX_BUNDLE Cone6.cxx)
target_link_libraries(Cone6 ${VTK_LIBRARIES})
