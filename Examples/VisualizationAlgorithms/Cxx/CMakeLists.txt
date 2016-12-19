cmake_minimum_required(VERSION 3.3 FATAL_ERROR)


project (VisualizationAlgorithms)
find_package(VTK REQUIRED)
vtk_module_config(VTK
  vtkCommonCore
  vtkCommonDataModel
  vtkFiltersCore
  vtkFiltersModeling
  vtkIOXML
  vtkInteractionStyle
  vtkRendering${VTK_RENDERING_BACKEND}
  vtkTestingRendering
)
include(${VTK_USE_FILE})

add_executable(TubesWithVaryingRadiusAndColors MACOSX_BUNDLE TubesWithVaryingRadiusAndColors.cxx)
add_executable(FilledContours MACOSX_BUNDLE FilledContours.cxx)
add_executable(BandedContours MACOSX_BUNDLE BandedContours.cxx)

target_link_libraries(TubesWithVaryingRadiusAndColors ${VTK_LIBRARIES})
target_link_libraries(FilledContours ${VTK_LIBRARIES})
target_link_libraries(BandedContours ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(vtk-example VisualizationAlgorithmsExamples)
    set(TestFilledContours_ARGS "DATA{${VTK_TEST_INPUT_DIR}/filledContours.vtp}" 10)
    set(TestBandedContours_ARGS "DATA{${VTK_TEST_INPUT_DIR}/filledContours.vtp}" 10)
    vtk_add_test_cxx(${vtk-example}CxxTests tests
      TestTubesWithVaryingRadiusAndColors.cxx
      TestFilledContours.cxx
      TestBandedContours.cxx
      )
    vtk_test_cxx_executable(${vtk-example}CxxTests tests
      RENDERING_FACTORY
      )
  endif()
endif()
