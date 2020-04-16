cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(ImageProcessing)

find_package(VTK
  OPTIONAL_COMPONENTS
    TestingCore
    TestingRendering)
find_package(VTK
  COMPONENTS
    CommonSystem
    IOImage
    ImagingFourier
    ImagingGeneral
    ImagingSources
    ImagingStatistics
    ImagingStencil
    InteractionStyle
    RenderingCore
    RenderingOpenGL2)
if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(ImageSlicing MACOSX_BUNDLE
  ImageSlicing.cxx)
target_link_libraries(ImageSlicing
  PRIVATE
    ${VTK_LIBRARIES})

add_executable(ImageBenchmark
  ImageBenchmark.cxx)
target_link_libraries(ImageBenchmark
  PRIVATE
    ${VTK_LIBRARIES})

add_executable(ImageBenchmarkDriver
  ImageBenchmarkDriver.cxx)
target_link_libraries(ImageBenchmarkDriver
  PRIVATE
    ${VTK_LIBRARIES})

vtk_module_autoinit(
  TARGETS ImageSlicing
          ImageBenchmark
          ImageBenchmarkDriver
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

    ExternalData_add_test(ImageProcessingData
      NAME    "ImageProcessingExampleCxx-ImageBenchmark"
      COMMAND ImageBenchmark
              --threads 2
              --filter resize:kernelsize=4
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    # FIXME: This test doesn't exit.
    if (FALSE)
      ExternalData_Expand_Arguments(ImageProcessingData _
        DATA{${CMAKE_CURRENT_SOURCE_DIR}/Testing/Data/headsq/,REGEX:.*})

      ExternalData_add_test(ImageProcessingData
        NAME    "ImageProcessingExampleCxx-ImageSlicing"
        COMMAND ImageSlicing
                ${CMAKE_CURRENT_BINARY_DIR}/ExternalData/Testing/Data/headsq/quarter
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif ()

    ExternalData_Add_Target(ImageProcessingData)
  endif ()
endif ()
