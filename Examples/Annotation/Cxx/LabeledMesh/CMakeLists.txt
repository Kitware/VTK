cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(LabeledMesh)

find_package(VTK
  COMPONENTS
    FiltersGeneral
    FiltersSources
    RenderingCore
    RenderingLabel)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(LabeledMesh MACOSX_BUNDLE
  LabeledMesh.cxx)
target_link_libraries(LabeledMesh
  PRIVATE
    ${VTK_LIBRARIES})
vtk_module_autoinit(
  TARGETS LabeledMesh
  MODULES ${VTK_LIBRARIES})
