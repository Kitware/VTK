cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT(ImageProcessing)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkCommonMath
    vtkIOImage
    vtkImagingCore
    vtkInteractionStyle
    vtkRenderingCore
    vtkRenderingOpenGL
  )
include(${VTK_USE_FILE})
endif()

add_executable(ImageSlicing MACOSX_BUNDLE ImageSlicing.cxx)
target_link_libraries(ImageSlicing ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
   ######## Regression Testing ########
   set(vtk-example ImageProcessingExamples)
   set(TestImageSlicing_ARGS ${VTK_TEST_DATA_DIR}/Data/headsq/quarter)
   vtk_add_test_cxx(
     TestImageSlicing.cxx
     )
   vtk_test_cxx_executable(${vtk-example}CxxTests RENDERING_FACTORY)
 endif()
endif()
