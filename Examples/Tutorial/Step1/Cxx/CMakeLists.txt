cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # remove if CMake >= 2.8.11 required
endif()

PROJECT (Step1)

if(NOT VTK_BINARY_DIR)
  FIND_PACKAGE(VTK COMPONENTS
    vtkFiltersSources
    vtkRenderingCore
    vtkRenderingOpenGL
  )
  include(${VTK_USE_FILE})
endif()

add_executable(Cone MACOSX_BUNDLE Cone.cxx)
target_link_libraries(Cone ${VTK_LIBRARIES})
