PROJECT (Widgets)
ENABLE_TESTING()

FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})

ADD_EXECUTABLE(Slider Slider.cxx)
TARGET_LINK_LIBRARIES(Slider vtkRendering vtkWidgets)

ADD_EXECUTABLE(Slider2D Slider2D.cxx)
TARGET_LINK_LIBRARIES(Slider2D vtkRendering vtkWidgets)

ADD_EXECUTABLE(BalloonWidget BalloonWidget.cxx)
TARGET_LINK_LIBRARIES(BalloonWidget vtkRendering vtkWidgets)

######## Regression Testing ########
SET(KIT WidgetsExamples)
SET(MyTests TestSlider.cxx TestSlider2D.cxx TestBalloonWidget.cxx)

SET(CMAKE_TESTDRIVER_BEFORE_TESTMAIN 
"    vtkTestingObjectFactory* factory = vtkTestingObjectFactory::New();
    vtkObjectFactory::RegisterFactory(factory);
    factory->Delete();"
)
SET(CMAKE_TESTDRIVER_AFTER_TESTMAIN
"    vtkObjectFactory::UnRegisterFactory(factory);"
)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx ${MyTests}
                       EXTRA_INCLUDE ${VTK_SOURCE_DIR}/Examples/vtkTestingObjectFactory.h)

ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests}
               ${VTK_SOURCE_DIR}/Examples/vtkTestingObjectFactory.cxx)
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkWidgets)

ADD_TEST(TestExampleSlider ${CXX_TEST_PATH}/${KIT}CxxTests TestSlider)
ADD_TEST(TestExampleSlider2D ${CXX_TEST_PATH}/${KIT}CxxTests TestSlider2D)
ADD_TEST(TestExampleBalloonWidget ${CXX_TEST_PATH}/${KIT}CxxTests TestBalloonWidget)
