cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (AMR)

if(NOT VTK_SOURCE_DIR)
  message(ERROR " Cannot build AMR examples without VTK_SOURCE_DIR")
endif()

find_package(VTK COMPONENTS
  vtkCommonCore
  vtkFiltersAMR
  vtkFiltersExtraction
  vtkFiltersGeometry
  vtkFiltersSources
  vtkIOLegacy
  vtkIOXML
  vtkInteractionStyle
  vtkRenderingOpenGL
  vtkTestingCore
)
include(${VTK_USE_FILE})

include_directories(
  ${VTK_SOURCE_DIR}/Testing/Core
)

## Add HierarchicalBoxPipeline executable
add_executable(HierarchicalBoxPipeline MACOSX_BUNDLE HierarchicalBoxPipeline.cxx)
target_link_libraries(HierarchicalBoxPipeline ${VTK_LIBRARIES})

## Add AMR examples
add_executable( Generate3DAMRDataSetWithPulse MACOSX_BUNDLE Generate3DAMRDataSetWithPulse.cxx)
target_link_libraries( Generate3DAMRDataSetWithPulse ${VTK_LIBRARIES})

add_executable( Generate2DAMRDataSetWithPulse MACOSX_BUNDLE Generate2DAMRDataSetWithPulse.cxx)
target_link_libraries(Generate2DAMRDataSetWithPulse ${VTK_LIBRARIES})
