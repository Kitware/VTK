cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT (DataManipulation)
find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkCommonDataModel
  vtkFiltersGeometry
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
)
include(${VTK_USE_FILE})

add_executable(Arrays MACOSX_BUNDLE Arrays.cxx)
add_executable(Cube MACOSX_BUNDLE Cube.cxx)
add_executable(SGrid MACOSX_BUNDLE SGrid.cxx)
add_executable(RGrid MACOSX_BUNDLE RGrid.cxx)

target_link_libraries(Arrays ${VTK_LIBRARIES})
target_link_libraries(Cube ${VTK_LIBRARIES})
target_link_libraries(SGrid ${VTK_LIBRARIES})
target_link_libraries(RGrid ${VTK_LIBRARIES})
