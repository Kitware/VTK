cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

project (Widgets)
find_package(VTK REQUIRED)
vtk_module_load(
  vtkCommonCore
  vtkFiltersSources
  vtkInteractionWidgets
  vtkRendering${VTK_RENDERING_BACKEND}
  vtkTestingRendering
)
include(${VTK_USE_FILE})


add_executable(Slider MACOSX_BUNDLE Slider.cxx)
target_link_libraries(Slider ${VTK_LIBRARIES})

add_executable(Slider2D MACOSX_BUNDLE Slider2D.cxx)
target_link_libraries(Slider2D ${VTK_LIBRARIES})

add_executable(BalloonWidget MACOSX_BUNDLE BalloonWidget.cxx)
target_link_libraries(BalloonWidget ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(vtk-example WidgetsExamples)
    vtk_add_test_cxx(${vtk-example}CxxTests tests
      TestSlider.cxx
      TestSlider2D.cxx
      TestBalloonWidget.cxx
      )
    vtk_test_cxx_executable(${vtk-example}CxxTests tests
      RENDERING_FACTORY
      )
  endif()
endif()
