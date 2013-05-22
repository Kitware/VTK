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
  set(vtk-example VisualizationAlgorithmsExamples)
  set(TestFilledContours_ARGS ${VTK_DATA_ROOT}/Data/filledContours.vtp 10)
  set(TestBandedContours_ARGS ${VTK_DATA_ROOT}/Data/filledContours.vtp 10)
  vtk_add_test_cxx(
    TestTubesWithVaryingRadiusAndColors.cxx
    TestFilledContours.cxx
    TestBandedContours.cxx
    )
  vtk_test_cxx_executable(${vtk-example}CxxTests RENDERING_FACTORY)
endif()
