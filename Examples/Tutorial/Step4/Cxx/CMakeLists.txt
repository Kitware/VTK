cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Step4)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkFiltersSources
    vtkRenderingCore
    vtkRenderingOpenGL
  )
include(${VTK_USE_FILE})
endif()

add_executable(Cone4 MACOSX_BUNDLE Cone4.cxx)
target_link_libraries(Cone4 ${VTK_LIBRARIES})
