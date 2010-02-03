SET(KIT IO)

SET(ConditionalTests)
IF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  SET(ConditionalTests ${ConditionalTests} TestAVIWriter.cxx)
ENDIF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
IF(VTK_USE_MPEG2_ENCODER)
  SET(ConditionalTests ${ConditionalTests} TestMovieWriter.cxx)
ENDIF(VTK_USE_MPEG2_ENCODER)
IF(VTK_USE_FFMPEG_ENCODER)
  SET(ConditionalTests ${ConditionalTests} TestFFMPEGWriter.cxx)
ENDIF(VTK_USE_FFMPEG_ENCODER)
IF(VTK_USE_DISPLAY AND VTK_USE_RENDERING)
  SET(ConditionalTests 
    ${ConditionalTests} 
    TestHyperOctreeIO.cxx 
    TestPLYReader.cxx
    SLACReaderLinear.cxx
    SLACReaderQuadratic.cxx
    )
    
IF(NOT WIN32)
  SET(ConditionalTests
    ${ConditionalTests}
    SLACParticleReader.cxx
    )
ENDIF(NOT WIN32)

ENDIF(VTK_USE_DISPLAY AND VTK_USE_RENDERING)
IF(VTK_USE_INFOVIS)
  SET(ConditionalTests ${ConditionalTests} TestDataObjectIO.cxx)
ENDIF(VTK_USE_INFOVIS)
IF(VTK_USE_METAIO)
  SET(ConditionalTests ${ConditionalTests} TestMetaIO.cxx)
ENDIF(VTK_USE_METAIO)
IF(VTK_USE_POSTGRES AND VTK_PSQL_TEST_URL AND POSTGRES_FOUND)
  SET(ConditionalTests ${ConditionalTests} TestPostgreSQLDatabase.cxx)
ENDIF(VTK_USE_POSTGRES AND VTK_PSQL_TEST_URL AND POSTGRES_FOUND)
IF(VTK_USE_MYSQL AND VTK_MYSQL_TEST_URL)
  SET(ConditionalTests ${ConditionalTests} TestMySQLDatabase.cxx)
ENDIF(VTK_USE_MYSQL AND VTK_MYSQL_TEST_URL)
IF(VTK_USE_ODBC AND VTK_ODBC_TEST_DSN)
  SET(ConditionalTests ${ConditionalTest} TestODBCDatabase.cxx)
ENDIF(VTK_USE_ODBC AND VTK_ODBC_TEST_DSN)
SET(VTK_HAS_UINT64_ARRAY 0)

IF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  SET(VTK_HAS_UINT64_ARRAY 1)
ELSE(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
  IF(VTK_SIZEOF_LONG MATCHES "^8$")
    SET(VTK_HAS_UINT64_ARRAY 1)
  ELSE(VTK_SIZEOF_LONG MATCHES "^8$")
    IF(VTK_SIZEOF___INT64 MATCHES "^8$")
      IF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
        SET(VTK_HAS_UINT64_ARRAY 1)
      ENDIF(VTK_TYPE_CONVERT_UI64_TO_DOUBLE)
    ENDIF(VTK_SIZEOF___INT64 MATCHES "^8$")
  ENDIF(VTK_SIZEOF_LONG MATCHES "^8$")
ENDIF(VTK_SIZEOF_LONG_LONG MATCHES "^8$")
IF(VTK_HAS_UINT64_ARRAY)
  SET(ConditionalTests ${ConditionalTests} TestSQLiteDatabase.cxx)
ENDIF(VTK_HAS_UINT64_ARRAY)
IF(VTK_USE_OGGTHEORA_ENCODER)
  SET(ConditionalTests ${ConditionalTests} TestOggTheoraWriter.cxx)
ENDIF(VTK_USE_OGGTHEORA_ENCODER)

CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  TestXML.cxx
  TestCompress.cxx
  TestSQLDatabaseSchema.cxx
  TestImageReader2Factory.cxx
  ${ConditionalTests}
  EXTRA_INCLUDE vtkTestDriver.h
)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} )

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkIO vtkImaging vtksys)

IF(VTK_USE_INFOVIS)
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkInfovis)
ENDIF(VTK_USE_INFOVIS)
IF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)
  TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkRendering)
ENDIF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)

IF (VTK_DATA_ROOT)
  ADD_TEST(TestXML ${CXX_TEST_PATH}/${KIT}CxxTests TestXML
    ${VTK_DATA_ROOT}/Data/sample.xml)
  ADD_TEST(TestImageReader2Factory ${CXX_TEST_PATH}/${KIT}CxxTests TestImageReader2Factory
    ${VTK_DATA_ROOT}/Baseline/${KIT}/TestHyperOctreeIO.png)
  IF(VTK_USE_METAIO)
    ADD_TEST(TestMetaIO ${CXX_TEST_PATH}/${KIT}CxxTests TestMetaIO
      ${VTK_DATA_ROOT}/Data/HeadMRVolume.mhd)
  ENDIF(VTK_USE_METAIO)
  ADD_TEST(TestCompress ${CXX_TEST_PATH}/${KIT}CxxTests TestCompress
    ${VTK_DATA_ROOT}/Data/sample.xml)
  IF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)
    ADD_TEST(TestHyperOctreeIO ${CXX_TEST_PATH}/${KIT}CxxTests
         TestHyperOctreeIO -D ${VTK_DATA_ROOT}
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -V ${VTK_DATA_ROOT}/Baseline/${KIT}/TestHyperOctreeIO.png)
    ADD_TEST(TestPLYReader ${CXX_TEST_PATH}/${KIT}CxxTests
         TestPLYReader -D ${VTK_DATA_ROOT}
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -V ${VTK_DATA_ROOT}/Baseline/${KIT}/TestPLYReader.png)
  ENDIF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)
ENDIF (VTK_DATA_ROOT)

IF (VTK_LARGE_DATA_ROOT)
  IF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)
    ADD_TEST(SLACReaderLinear ${CXX_TEST_PATH}/${KIT}CxxTests
         SLACReaderLinear -D ${VTK_LARGE_DATA_ROOT}
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -V ${VTK_LARGE_DATA_ROOT}/Baseline/${KIT}/SLACReaderLinear.png)
    ADD_TEST(SLACReaderQuadratic ${CXX_TEST_PATH}/${KIT}CxxTests
         SLACReaderQuadratic -D ${VTK_LARGE_DATA_ROOT}
         -T ${VTK_BINARY_DIR}/Testing/Temporary
         -V ${VTK_LARGE_DATA_ROOT}/Baseline/${KIT}/SLACReaderQuadratic.png)
    IF(NOT WIN32)
      ADD_TEST(SLACParticleReader ${CXX_TEST_PATH}/${KIT}CxxTests
           SLACParticleReader -D ${VTK_LARGE_DATA_ROOT}
           -T ${VTK_BINARY_DIR}/Testing/Temporary
           -V ${VTK_LARGE_DATA_ROOT}/Baseline/${KIT}/SLACParticleReader.png)
    ENDIF(NOT WIN32)
  ENDIF (VTK_USE_DISPLAY AND VTK_USE_RENDERING)
ENDIF (VTK_LARGE_DATA_ROOT)

ADD_TEST(TestSQLDatabaseSchema ${CXX_TEST_PATH}/${KIT}CxxTests TestSQLDatabaseSchema)

IF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
  ADD_TEST(TestAVIWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestAVIWriter)
ENDIF(WIN32 AND VTK_USE_VIDEO_FOR_WINDOWS)
IF(VTK_USE_MPEG2_ENCODER)
  ADD_TEST(TestMovieWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestMovieWriter)
ENDIF(VTK_USE_MPEG2_ENCODER)
IF(VTK_USE_FFMPEG_ENCODER)
  ADD_TEST(TestFFMPEGWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestFFMPEGWriter)
ENDIF(VTK_USE_FFMPEG_ENCODER)
IF(VTK_HAS_UINT64_ARRAY)
  ADD_TEST(TestSQLiteDatabase ${CXX_TEST_PATH}/${KIT}CxxTests TestSQLiteDatabase)
ENDIF(VTK_HAS_UINT64_ARRAY)
IF(VTK_USE_POSTGRES AND POSTGRES_FOUND AND VTK_PSQL_TEST_URL)
  ADD_TEST(TestPostgreSQLDatabase ${CXX_TEST_PATH}/${KIT}CxxTests TestPostgreSQLDatabase)
ENDIF(VTK_USE_POSTGRES AND POSTGRES_FOUND AND VTK_PSQL_TEST_URL)
IF(VTK_USE_MYSQL AND VTK_MYSQL_TEST_URL)
  ADD_TEST(TestMySQLDatabase ${CXX_TEST_PATH}/${KIT}CxxTests TestMySQLDatabase)
ENDIF(VTK_USE_MYSQL AND VTK_MYSQL_TEST_URL)
IF(VTK_USE_OGGTHEORA_ENCODER)
  ADD_TEST(TestOggTheoraWriter ${CXX_TEST_PATH}/${KIT}CxxTests TestOggTheoraWriter)
ENDIF(VTK_USE_OGGTHEORA_ENCODER)

IF(VTK_USE_INFOVIS)
  ADD_TEST(TestDataObjectIO ${CXX_TEST_PATH}/${KIT}CxxTests TestDataObjectIO)
ENDIF(VTK_USE_INFOVIS)

IF(VTK_USE_N_WAY_ARRAYS)

  CREATE_TEST_SOURCELIST(ArrayIOTests ArrayIOCxxTests.cxx
    TestArraySerialization.cxx
    EXTRA_INCLUDE vtkTestDriver.h
    )

  ADD_EXECUTABLE(ArrayIOCxxTests ${ArrayIOTests})
  TARGET_LINK_LIBRARIES(ArrayIOCxxTests vtkIO)

  SET(ArrayTestsToRun ${ArrayIOTests})
  REMOVE(ArrayTestsToRun ArrayIOCxxTests.cxx)

  FOREACH(test ${ArrayTestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    ADD_TEST(Array-${TName} ${CXX_TEST_PATH}/ArrayIOCxxTests ${TName})
  ENDFOREACH(test)


ENDIF(VTK_USE_N_WAY_ARRAYS)
