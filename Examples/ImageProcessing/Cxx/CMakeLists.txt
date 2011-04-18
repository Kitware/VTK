PROJECT(ImageProcessing)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(ImageSlicing ImageSlicing.cxx)
TARGET_LINK_LIBRARIES(ImageSlicing vtkRendering vtkImaging vtkIO)

######## Regression Testing ########
SET(KIT ImageProcessingExamples)
SET(MyTests TestImageSlicing.cxx)

INCLUDE(vtkTestingObjectFactory)
ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkIO)

ADD_TEST(${KIT}-ImageSlicing ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
  TestImageSlicing ${VTK_DATA_ROOT}/Data/headsq/quarter)


