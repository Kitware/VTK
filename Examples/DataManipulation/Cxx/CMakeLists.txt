cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(DataManipulation)

find_package(VTK
  COMPONENTS
    CommonColor
    FiltersGeometry
    RenderingCore)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(Arrays MACOSX_BUNDLE Arrays.cxx)
add_executable(Cube MACOSX_BUNDLE Cube.cxx)
add_executable(SGrid MACOSX_BUNDLE SGrid.cxx)
add_executable(RGrid MACOSX_BUNDLE RGrid.cxx)

target_link_libraries(Arrays PRIVATE ${VTK_LIBRARIES})
target_link_libraries(Cube PRIVATE ${VTK_LIBRARIES})
target_link_libraries(SGrid PRIVATE ${VTK_LIBRARIES})
target_link_libraries(RGrid PRIVATE ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS Arrays
          Cube
          SGrid
          RGrid
  MODULES ${VTK_LIBRARIES})
