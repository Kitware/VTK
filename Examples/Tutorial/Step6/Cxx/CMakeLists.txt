cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Step6)

if(NOT VTK_BINARY_DIR)
  FIND_PACKAGE(VTK COMPONENTS
    vtkCommonCore
    vtkCommonTransforms
    vtkFiltersSources
    vtkInteractionStyle
    vtkInteractionWidgets
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkInteractionStyle
)
  include(${VTK_USE_FILE})
endif()

add_executable(Cone6 MACOSX_BUNDLE Cone6.cxx)
target_link_libraries(Cone6 ${VTK_LIBRARIES})
