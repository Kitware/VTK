cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()

project (Step4)

find_package(VTK COMPONENTS
  vtkFiltersSources
  vtkInteractionStyle
  vtkRenderingOpenGL
)
include(${VTK_USE_FILE})

add_executable(Cone4 MACOSX_BUNDLE Cone4.cxx)
target_link_libraries(Cone4 ${VTK_LIBRARIES})
