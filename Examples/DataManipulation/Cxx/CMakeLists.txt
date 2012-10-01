cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (DataManipulation)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkFiltersCore
    vtkFiltersGeometry
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkInteractionStyle
)
include(${VTK_USE_FILE})
endif()

add_executable(Arrays MACOSX_BUNDLE Arrays.cxx)
add_executable(Cube MACOSX_BUNDLE Cube.cxx)
add_executable(SGrid MACOSX_BUNDLE SGrid.cxx)
add_executable(RGrid MACOSX_BUNDLE RGrid.cxx)

target_link_libraries(Arrays ${VTK_LIBRARIES})
target_link_libraries(Cube ${VTK_LIBRARIES})
target_link_libraries(SGrid ${VTK_LIBRARIES})
target_link_libraries(RGrid ${VTK_LIBRARIES})
