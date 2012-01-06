PROJECT (VisualizationAlgorithms)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(TubesWithVaryingRadiusAndColors TubesWithVaryingRadiusAndColors.cxx)
ADD_EXECUTABLE(FilledContours FilledContours.cxx)
ADD_EXECUTABLE(BandedContours BandedContours.cxx)

TARGET_LINK_LIBRARIES(TubesWithVaryingRadiusAndColors vtkHybrid)
TARGET_LINK_LIBRARIES(FilledContours vtkHybrid)
TARGET_LINK_LIBRARIES(BandedContours vtkHybrid)

######## Regression Testing ########
SET(KIT VisualizationAlgorithmsExamples)
SET(MyTests
    TestFilledContours.cxx
    TestBandedContours.cxx
)

IF (VTK_DATA_ROOT)
  SET(MyTests ${MyTests} TestTubesWithVaryingRadiusAndColors.cxx)
ENDIF (VTK_DATA_ROOT)

INCLUDE(vtkTestingObjectFactory)
ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkHybrid)

ADD_TEST(${KIT}-TubesWithVaryingRadiusAndColors
         ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestTubesWithVaryingRadiusAndColors)
ADD_TEST(${KIT}-FilledContours
         ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestFilledContours
         ${VTK_DATA_ROOT}/Data/filledContours.vtp 10)
ADD_TEST(${KIT}-BandedContours
         ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestBandedContours
         ${VTK_DATA_ROOT}/Data/filledContours.vtp 10)
