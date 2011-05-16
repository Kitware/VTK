PROJECT (Medical)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
IF(NOT VTK_USE_RENDERING)
  MESSAGE(FATAL_ERROR "Example ${PROJECT_NAME} requires VTK_USE_RENDERING.")
ENDIF(NOT VTK_USE_RENDERING)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(Medical1 Medical1.cxx)
ADD_EXECUTABLE(Medical2 Medical2.cxx)
ADD_EXECUTABLE(Medical3 Medical3.cxx)
ADD_EXECUTABLE(Medical4 Medical4.cxx)

TARGET_LINK_LIBRARIES(Medical1 vtkRendering vtkIO)
TARGET_LINK_LIBRARIES(Medical2 vtkRendering vtkIO)
TARGET_LINK_LIBRARIES(Medical3 vtkRendering vtkIO)
TARGET_LINK_LIBRARIES(Medical4 vtkVolumeRendering vtkIO)

IF(VTK_USE_METAIO)
  ADD_EXECUTABLE(GenerateModelsFromLabels GenerateModelsFromLabels.cxx)
  ADD_EXECUTABLE(GenerateCubesFromLabels GenerateCubesFromLabels.cxx)
  TARGET_LINK_LIBRARIES(GenerateModelsFromLabels vtkHybrid vtkIO)
  TARGET_LINK_LIBRARIES(GenerateCubesFromLabels vtkHybrid vtkIO)
ENDIF(VTK_USE_METAIO)

######## Regression Testing ########
SET(KIT MedicalExamples)
SET(MyTests TestMedical1.cxx TestMedical2.cxx TestMedical3.cxx TestMedical4.cxx)

INCLUDE(vtkTestingObjectFactory)
ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkWidgets vtkVolumeRendering)

ADD_TEST(${KIT}-Medical1 ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
  TestMedical1 ${VTK_DATA_ROOT}/Data/headsq/quarter)
ADD_TEST(${KIT}-Medical2 ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
  TestMedical2 ${VTK_DATA_ROOT}/Data/headsq/quarter)
ADD_TEST(${KIT}-Medical3 ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
  TestMedical3 ${VTK_DATA_ROOT}/Data/headsq/quarter)
ADD_TEST(${KIT}-Medical4 ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
  TestMedical4 ${VTK_DATA_ROOT}/Data/headsq/quarter)
