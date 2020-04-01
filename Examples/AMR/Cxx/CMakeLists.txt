cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(AMR)

find_package(VTK
  COMPONENTS
    CommonDataModel
    FiltersExtraction
    FiltersGeometry
    FiltersSources
    IOLegacy
    IOXML
    RenderingCore
    TestingCore)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

## Add HierarchicalBoxPipeline executable
add_executable(HierarchicalBoxPipeline MACOSX_BUNDLE
  HierarchicalBoxPipeline.cxx)
target_link_libraries(HierarchicalBoxPipeline
  PRIVATE
    ${VTK_LIBRARIES})

## Add AMR examples
add_executable(Generate3DAMRDataSetWithPulse MACOSX_BUNDLE
  Generate3DAMRDataSetWithPulse.cxx)
target_link_libraries(Generate3DAMRDataSetWithPulse
  PRIVATE
    ${VTK_LIBRARIES})

add_executable(Generate2DAMRDataSetWithPulse MACOSX_BUNDLE
  Generate2DAMRDataSetWithPulse.cxx)
target_link_libraries(Generate2DAMRDataSetWithPulse
  PRIVATE
    ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS HierarchicalBoxPipeline
          Generate3DAMRDataSetWithPulse
          Generate2DAMRDataSetWithPulse
  MODULES ${VTK_LIBRARIES})
