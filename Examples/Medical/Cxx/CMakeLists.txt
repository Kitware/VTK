cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
project(Medical)

find_package(VTK
  COMPONENTS
    FiltersGeometry
    FiltersModeling
    IOImage
    IOXML
    ImagingStatistics
    ImagingCore
    RenderingCore
    RenderingVolume
  OPTIONAL_COMPONENTS
    TestingRendering)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(Medical1 MACOSX_BUNDLE Medical1.cxx)
add_executable(Medical2 MACOSX_BUNDLE Medical2.cxx)
add_executable(Medical3 MACOSX_BUNDLE Medical3.cxx)
add_executable(Medical4 MACOSX_BUNDLE Medical4.cxx)

target_link_libraries(Medical1 PRIVATE ${VTK_LIBRARIES})
target_link_libraries(Medical2 PRIVATE ${VTK_LIBRARIES})
target_link_libraries(Medical3 PRIVATE ${VTK_LIBRARIES})
target_link_libraries(Medical4 PRIVATE ${VTK_LIBRARIES})

add_executable(GenerateModelsFromLabels MACOSX_BUNDLE GenerateModelsFromLabels.cxx)
add_executable(GenerateCubesFromLabels MACOSX_BUNDLE  GenerateCubesFromLabels.cxx)
target_link_libraries(GenerateModelsFromLabels PRIVATE ${VTK_LIBRARIES})
target_link_libraries(GenerateCubesFromLabels PRIVATE ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS Medical1
          Medical2
          Medical3
          Medical4
          GenerateModelsFromLabels
          GenerateCubesFromLabels
  MODULES ${vtk_modules})

include(CTest)

if (TARGET VTK::TestingRendering)
  add_subdirectory(Testing)
endif ()
