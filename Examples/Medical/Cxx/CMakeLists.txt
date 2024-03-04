cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(Medical)

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")

find_package(VTK
  COMPONENTS
    CommonCore
    CommonDataModel
    CommonTransforms
    FiltersCore
    FiltersGeneral
    FiltersGeometry
    FiltersModeling
    IOImage
    IOXML
    ImagingCore
    ImagingStatistics
    InteractionStyle
    RenderingCore
    RenderingVolume
    RenderingOpenGL2
    RenderingVolumeOpenGL2
  OPTIONAL_COMPONENTS
    TestingCore
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
  MODULES ${VTK_LIBRARIES})

if (BUILD_TESTING)
  include(CTest)
  if (TARGET VTK::TestingRendering)

    ######## Regression Testing ########
    set(ExternalData_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR}/ExternalData)

    list(APPEND ExternalData_URL_TEMPLATES
      # Data published by Girder
      "https://data.kitware.com/api/v1/file/hashsum/%(algo)/%(hash)/download"

      # Data published by developers using git-gitlab-push.
      "https://www.vtk.org/files/ExternalData/%(algo)/%(hash)"
    )
    include(ExternalData)

    ExternalData_Expand_Arguments(MedicalData _
      DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/headsq/,REGEX:.*})

    foreach(test Medical1 Medical2 Medical3 Medical4)
      ExternalData_add_test(MedicalData
        NAME    "MedicalExampleCxx-${test}"
        COMMAND ${test}
          ${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing/Data/headsq/quarter
          -V DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/Baseline/${test}.png}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
      set_property(TEST "MedicalExampleCxx-${test}"
        PROPERTY
          ENVIRONMENT "VTK_TESTING=1;VTK_TESTING_IMAGE_COMPARE_METHOD=TIGHT_VALID")
    endforeach()

    ExternalData_Add_Target(MedicalData)
  endif ()
endif ()
