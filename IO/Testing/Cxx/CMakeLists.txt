SET(KIT IO)

SET(ConditionalTests)
IF(VTK_USE_VIDEO_FOR_WINDOWS)
  SET(ConditionalTests TestAVIWriter.cxx)
ENDIF(VTK_USE_VIDEO_FOR_WINDOWS)

CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  TestXML.cxx
  TestCompress.cxx
  TestMovieWriter.cxx
  ${ConditionalTests}
  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkIO vtkImaging vtksys)

IF (VTK_DATA_ROOT)
  ADD_TEST(TestXML ${CXX_TEST_PATH}/${KIT}CxxTests TestXML ${VTK_DATA_ROOT}/Data/sample.xml)
  ADD_TEST(TestCompress ${CXX_TEST_PATH}/${KIT}CxxTests TestCompress ${VTK_DATA_ROOT}/Data/sample.xml)
ENDIF (VTK_DATA_ROOT)

ADD_TEST(TestMovieWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestMovieWriter)

IF(ConditionalTests)
  ADD_TEST(TestAVIWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestAVIWriter)
ENDIF(ConditionalTests)
