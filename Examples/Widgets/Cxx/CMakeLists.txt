cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

PROJECT (Widgets)

if(NOT VTK_BINARY_DIR)
  find_package(VTK COMPONENTS
    vtkCommonComputationalGeometry
    vtkCommonCore
    vtkCommonDataModel
    vtkFiltersSources
    vtkInteractionStyle
    vtkInteractionWidgets
    vtkRenderingCore
    vtkRenderingOpenGL
    vtkRenderingFreeTypeOpenGL
)
  include(${VTK_USE_FILE})
endif()

add_executable(Slider MACOSX_BUNDLE Slider.cxx)
target_link_libraries(Slider ${VTK_LIBRARIES})

add_executable(Slider2D MACOSX_BUNDLE Slider2D.cxx)
target_link_libraries(Slider2D ${VTK_LIBRARIES})

add_executable(BalloonWidget MACOSX_BUNDLE BalloonWidget.cxx)
target_link_libraries(BalloonWidget ${VTK_LIBRARIES})

if(BUILD_TESTING)
  if(vtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(KIT WidgetsExamples)
    set(MyTests TestSlider.cxx TestSlider2D.cxx TestBalloonWidget.cxx)

    include(vtkTestingObjectFactory)

    add_executable(${KIT}CxxTests ${KIT}CxxTests.cxx ${MyTests})
    target_link_libraries(${KIT}CxxTests ${VTK_LIBRARIES})

    add_test(NAME ${KIT}-Slider COMMAND ${KIT}CxxTests TestSlider)
    add_test(NAME ${KIT}-Slider2D COMMAND ${KIT}CxxTests TestSlider2D)
    add_test(NAME ${KIT}-BalloonWidget COMMAND ${KIT}CxxTests TestBalloonWidget)
  endif()
endif()
