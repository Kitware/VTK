SET(KIT IO)

SET(ConditionalTests)
IF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  SET(ConditionalTests ${ConditionalTests} TestAVIWriter.cxx)
ENDIF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
IF(VTK_USE_MPEG2_ENCODER)
  SET(ConditionalTests ${ConditionalTests} TestMovieWriter.cxx)
ENDIF(VTK_USE_MPEG2_ENCODER)

CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  TestXML.cxx
  TestCompress.cxx
  ${ConditionalTests}
  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkIO vtkImaging vtksys)

IF (VTK_DATA_ROOT)
  ADD_TEST(TestXML ${CXX_TEST_PATH}/${KIT}CxxTests TestXML ${VTK_DATA_ROOT}/Data/sample.xml)
  ADD_TEST(TestCompress ${CXX_TEST_PATH}/${KIT}CxxTests TestCompress ${VTK_DATA_ROOT}/Data/sample.xml)
ENDIF (VTK_DATA_ROOT)

IF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  ADD_TEST(TestAVIWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestAVIWriter)
ENDIF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
IF(VTK_USE_MPEG2_ENCODER)
  ADD_TEST(TestMovieWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestMovieWriter)
ENDIF(VTK_USE_MPEG2_ENCODER)
