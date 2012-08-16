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

if(vtkTestingRendering_SOURCE_DIR)
######## Regression Testing ########
set(KIT ImageProcessingExamples)
set(MyTests TestImageSlicing.cxx)

include("${vtkTestingRendering_SOURCE_DIR}/vtkTestingObjectFactory.cmake")
add_executable(${KIT}CxxTests ${KIT}CxxTests.cxx ${MyTests})
target_link_libraries(${KIT}CxxTests ${VTK_LIBRARIES})

  if (VTK_DATA_ROOT)
    add_test(NAME ${KIT}-ImageSlicing COMMAND ${KIT}CxxTests
      TestImageSlicing ${VTK_DATA_ROOT}/Data/headsq/quarter)
  endif()
endif()
