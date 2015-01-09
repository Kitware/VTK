cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

PROJECT(ImageProcessing)

find_package(VTK COMPONENTS
  vtkIOImage
  vtkImagingCore
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
  vtkTestingRendering
)
include(${VTK_USE_FILE})

add_executable(ImageSlicing MACOSX_BUNDLE ImageSlicing.cxx)
target_link_libraries(ImageSlicing ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
   ######## Regression Testing ########
   set(vtk-example ImageProcessingExamples)
   set(TestImageSlicing_ARGS ${VTK_TEST_DATA_DIR}/Data/headsq/quarter)
   vtk_add_test_cxx(${vtk-example}CxxTests tests
     TestImageSlicing.cxx
     )
   vtk_test_cxx_executable(${vtk-example}CxxTests tests
     RENDERING_FACTORY
     )
 endif()
endif()
