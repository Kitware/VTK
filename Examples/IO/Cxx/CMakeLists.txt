PROJECT (IO)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(DumpXMLFile DumpXMLFile.cxx)
TARGET_LINK_LIBRARIES(DumpXMLFile vtkIO)

ADD_EXECUTABLE(ReadXYZAFile ReadXYZAFile.cxx)
TARGET_LINK_LIBRARIES(ReadXYZAFile vtkHybrid)

######## Regression Testing ########
SET(KIT IOExamples)
SET(MyTests TestDumpXMLFile.cxx TestReadXYZAFile.cxx)

INCLUDE(${VTK_SOURCE_DIR}/Examples/vtkTestingObjectFactory.cmake)

ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests}
               ${VTK_SOURCE_DIR}/Examples/vtkTestingObjectFactory.cxx)
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkWidgets)

IF (VTK_DATA_ROOT)
ADD_TEST(${KIT}-ReadXYZAFile ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestReadXYZAFile
           ${VTK_DATA_ROOT}/Data/golf.csv
           ${VTK_BINARY_DIR}/Testing/Temporary/TestReadXYZAFile.vtp)
ADD_TEST(${KIT}-DumpXMLFile ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestDumpXMLFile
           ${VTK_DATA_ROOT}/Data/cow.vtp)
ENDIF (VTK_DATA_ROOT)

