cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

project (Step3)

find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
)
include(${VTK_USE_FILE})

add_executable(Cone3 MACOSX_BUNDLE Cone3.cxx)
target_link_libraries(Cone3 ${VTK_LIBRARIES})
