cmake_minimum_required(VERSION 3.3...3.12 FATAL_ERROR)
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
    RenderingCore)
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
  if (TARGET VTK::TestingRendering)
    ######## Regression Testing ########
    set(_vtk_test_module ImageProcessingExamples)
    vtk_module_test_data(
      Data/headsq/quarter/,REGEX:.*)
    set(TestImageSlicing_ARGS ${_vtk_build_TEST_DATA_OUTPUT_DIRECTORY}/Data/headsq/quarter)
    set(TestImageBenchmark_ARGS
      "--threads" "2" "--filter" "resize:kernelsize=4")
    vtk_add_test_cxx(ImageProcessingExamplesCxxTests tests
      TestImageBenchmark.cxx,NO_VALID,NO_DATA,NO_OUTPUT
      TestImageSlicing.cxx
      )
    vtk_test_cxx_executable(ImageProcessingExamplesCxxTests tests
      RENDERING_FACTORY
      )
  endif ()
endif ()
