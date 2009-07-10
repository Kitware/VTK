# List tests that can be built and run automatically ...
SET(AUTO_TESTS
  TestNGramExtraction
  TestTermDictionary
  TestTokenLengthFilter
  TestTokenValueFilter
  TestTokenizer
  TestTokenizerRanges
  )

SET(AUTO_TEST_LIBRARIES
  vtkCommon
  vtkTextAnalysis
  )

# Build the auto test executable ...
FOREACH(TEST ${AUTO_TESTS})
  LIST(APPEND TEST_SOURCES ${TEST}.cxx)
ENDFOREACH()

CREATE_TEST_SOURCELIST(TEST_SOURCES CxxTests.cxx ${TEST_SOURCES} EXTRA_INCLUDE vtkTestDriver.h)

#INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Common)
ADD_EXECUTABLE(TextAnalysisCxxTests ${TEST_SOURCES})
TARGET_LINK_LIBRARIES(TextAnalysisCxxTests ${AUTO_TEST_LIBRARIES})
GET_TARGET_PROPERTY(TEST_LOCATION TextAnalysisCxxTests LOCATION)

# Setup automatially-run tests ...
FOREACH(TEST ${AUTO_TESTS})
  ADD_TEST("Text-${TEST}" ${TEST_LOCATION} ${TEST})
  ENDFOREACH()

