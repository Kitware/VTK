PROJECT (VisualizationAlgorithms)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(TubesWithVaryingRadiusAndColors TubesWithVaryingRadiusAndColors.cxx)

TARGET_LINK_LIBRARIES(TubesWithVaryingRadiusAndColors vtkHybrid)

######## Regression Testing ########
SET(KIT VisualizationAlgorithmsExamples)
SET(MyTests TestTubesWithVaryingRadiusAndColors.cxx)

INCLUDE(${VTK_SOURCE_DIR}/Rendering/vtkTestingObjectFactory.cmake)
ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkHybrid)

ADD_TEST(${KIT}-TubesWithVaryingRadiusAndColors
         ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestTubesWithVaryingRadiusAndColors)
