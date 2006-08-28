SET(KIT IO)

SET(ConditionalTests)
IF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  SET(ConditionalTests TestAVIWriter.cxx)
ENDIF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
IF(DEFINED vtkMPEG2Encode_INCLUDE_PATH)
  SET(ConditionalTests ${ConditionalTests} TestMovieWriter.cxx)
ENDIF(DEFINED vtkMPEG2Encode_INCLUDE_PATH)
IF(VTK_USE_FFMPEG_ENCODER)
  SET(ConditionalTests ${ConditionalTests} TestFFMPEGWriter.cxx)
ENDIF(VTK_USE_FFMPEG_ENCODER)
IF(VTK_USE_RENDERING)
  SET(ConditionalTests ${ConditionalTests} TestHyperOctreeIO.cxx)
ENDIF(VTK_USE_RENDERING)


CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  TestXML.cxx
  TestCompress.cxx
  ${ConditionalTests}
  EXTRA_INCLUDE vtkTestDriver.h
)
ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkIO vtkImaging vtksys)

IF (VTK_DATA_ROOT)
  ADD_TEST(TestXML ${CXX_TEST_PATH}/${KIT}CxxTests TestXML
    ${VTK_DATA_ROOT}/Data/sample.xml)
  ADD_TEST(TestCompress ${CXX_TEST_PATH}/${KIT}CxxTests TestCompress
    ${VTK_DATA_ROOT}/Data/sample.xml)
  IF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)
    TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering)
    ADD_TEST(TestHyperOctreeIO ${CXX_TEST_PATH}/${KIT}CxxTests
         TestHyperOctreeIO -D ${VTK_DATA_ROOT}
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -V ${VTK_DATA_ROOT}/Baseline/${KIT}/TestHyperOctreeIO.png)
  ENDIF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)
ENDIF (VTK_DATA_ROOT)

IF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  ADD_TEST(TestAVIWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestAVIWriter)
ENDIF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
IF(DEFINED vtkMPEG2Encode_INCLUDE_PATH)
  ADD_TEST(TestMovieWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestMovieWriter)
ENDIF(DEFINED vtkMPEG2Encode_INCLUDE_PATH)

IF(VTK_USE_FFMPEG_ENCODER)
  ADD_TEST(TestFFMPEGWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestFFMPEGWriter)
ENDIF(VTK_USE_FFMPEG_ENCODER)
