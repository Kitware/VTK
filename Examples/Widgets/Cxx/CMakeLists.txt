PROJECT (Widgets)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(Slider Slider.cxx)
TARGET_LINK_LIBRARIES(Slider vtkRendering vtkWidgets)

ADD_EXECUTABLE(Slider2D Slider2D.cxx)
TARGET_LINK_LIBRARIES(Slider2D vtkRendering vtkWidgets)

ADD_EXECUTABLE(BalloonWidget BalloonWidget.cxx)
TARGET_LINK_LIBRARIES(BalloonWidget vtkRendering vtkWidgets)

######## Regression Testing ########
SET(KIT WidgetsExamples)
SET(MyTests TestSlider.cxx TestSlider2D.cxx TestBalloonWidget.cxx)

INCLUDE(vtkTestingObjectFactory)

ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkWidgets)

IF (VTK_DATA_ROOT)
  ADD_TEST(${KIT}-Slider ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests TestSlider)
  ADD_TEST(${KIT}-Slider2D ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests TestSlider2D)
  ADD_TEST(${KIT}-BalloonWidget ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests TestBalloonWidget)
ENDIF (VTK_DATA_ROOT)
