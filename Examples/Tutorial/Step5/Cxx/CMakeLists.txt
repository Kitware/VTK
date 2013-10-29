cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # remove if CMake >= 2.8.11 required
endif()

PROJECT (Step5)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkFiltersSources
    vtkInteractionStyle
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkInteractionStyle
  )
  include(${VTK_USE_FILE})
endif()

add_executable(Cone5 MACOSX_BUNDLE Cone5.cxx)
target_link_libraries(Cone5 ${VTK_LIBRARIES})
