cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
PROJECT (Step2)

IF(NOT VTK_BINARY_DIR)
  FIND_PACKAGE(VTK COMPONENTS
    vtkCommonCore
    vtkFiltersSources
    vtkRenderingCore
    vtkRenderingOpenGL
  )
  include(${VTK_USE_FILE})
endif()

add_executable(Cone2 MACOSX_BUNDLE Cone2.cxx)
target_link_libraries(Cone2 ${VTK_LIBRARIES})
