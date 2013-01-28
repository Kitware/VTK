cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (VisualizationAlgorithms)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonCore
    vtkCommonDataModel
    vtkFiltersCore
    vtkFiltersModeling
    vtkIOXML
    vtkInteractionStyle
    vtkRenderingCore
    vtkRenderingOpenGL
)
  include(${VTK_USE_FILE})
endif()

add_executable(TubesWithVaryingRadiusAndColors MACOSX_BUNDLE TubesWithVaryingRadiusAndColors.cxx)
add_executable(FilledContours MACOSX_BUNDLE FilledContours.cxx)
add_executable(BandedContours MACOSX_BUNDLE BandedContours.cxx)

target_link_libraries(TubesWithVaryingRadiusAndColors ${VTK_LIBRARIES})
target_link_libraries(FilledContours ${VTK_LIBRARIES})
target_link_libraries(BandedContours ${VTK_LIBRARIES})

if(vtkTestingRendering_SOURCE_DIR)
######## Regression Testing ########
set(KIT VisualizationAlgorithmsExamples)
set(MyTests
    TestFilledContours.cxx
    TestBandedContours.cxx
    TestTubesWithVaryingRadiusAndColors.cxx
)

include(vtkTestingObjectFactory)

add_executable(${KIT}CxxTests ${KIT}CxxTests.cxx ${MyTests})
target_link_libraries(${KIT}CxxTests ${VTK_LIBRARIES})

add_test(NAME ${KIT}-TubesWithVaryingRadiusAndColors
         COMMAND ${KIT}CxxTests
         TestTubesWithVaryingRadiusAndColors)
  if (VTK_DATA_ROOT)
    add_test(NAME ${KIT}-FilledContours
             COMMAND ${KIT}CxxTests
             TestFilledContours
               ${VTK_DATA_ROOT}/Data/filledContours.vtp 10)
    add_test(NAME ${KIT}-BandedContours
             COMMAND ${KIT}CxxTests
             TestBandedContours
               ${VTK_DATA_ROOT}/Data/filledContours.vtp 10)
  endif()
endif()
