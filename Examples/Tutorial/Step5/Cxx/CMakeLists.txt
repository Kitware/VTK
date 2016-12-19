cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


PROJECT (Step5)

find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
)
include(${VTK_USE_FILE})

add_executable(Cone5 MACOSX_BUNDLE Cone5.cxx)
target_link_libraries(Cone5 ${VTK_LIBRARIES})
