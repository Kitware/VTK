PROJECT (IO)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)

ADD_EXECUTABLE(DumpXMLFile DumpXMLFile.cxx)
TARGET_LINK_LIBRARIES(DumpXMLFile vtkIO vtkGraphics)

ADD_EXECUTABLE(ParticleReader ParticleReader.cxx)
TARGET_LINK_LIBRARIES(ParticleReader vtkHybrid)

######## Regression Testing ########
SET(KIT IOExamples)
SET(MyTests TestDumpXMLFile.cxx TestParticleReader.cxx)

INCLUDE(vtkTestingObjectFactory)

ADD_EXECUTABLE(${KIT}CxxTests ${KIT}CxxTests.cxx
               ${MyTests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering vtkWidgets)

IF (VTK_DATA_ROOT)
ADD_TEST(${KIT}-ParticleReader ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestParticleReader
           ${VTK_DATA_ROOT}/Data/golf.csv
           ${VTK_BINARY_DIR}/Testing/Temporary/TestParticleReader.vtp)
ADD_TEST(${KIT}-DumpXMLFile ${EXECUTABLE_OUTPUT_PATH}/${KIT}CxxTests
         TestDumpXMLFile
           ${VTK_DATA_ROOT}/Data/cow.vtp)
ENDIF (VTK_DATA_ROOT)

