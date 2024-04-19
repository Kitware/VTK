cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(MultiBlock)

find_package(VTK
  COMPONENTS
    FiltersGeneral
    FiltersGeometry
    FiltersSources
    IOXML
    RenderingCore
    TestingCore)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(MultiBlock MACOSX_BUNDLE MultiBlock.cxx)
target_link_libraries(MultiBlock
  PRIVATE
    ${VTK_LIBRARIES})
vtk_module_autoinit(
  TARGETS MultiBlock
  MODULES ${VTK_LIBRARIES})
