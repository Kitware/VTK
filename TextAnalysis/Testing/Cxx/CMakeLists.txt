# List tests that can be built and run automatically ...
SET(AUTO_TESTS
  TestAssignMimeType
  TestMimeTypes
  TestNGramExtraction
  TestTermDictionary
  TestTextExtraction
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

CREATE_TEST_SOURCELIST(TEST_SOURCES ${KIT}CxxTests.cxx 
  ${TEST_SOURCES} 
  EXTRA_INCLUDE vtkTestDriver.h
)

#INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Common)
ADD_EXECUTABLE(${KIT}CxxTests ${TEST_SOURCES})
TARGET_LINK_LIBRARIES(${KIT}CxxTests ${AUTO_TEST_LIBRARIES})

# Setup automatially-run tests ...
FOREACH(TEST ${AUTO_TESTS})
  ADD_TEST("Text-${TEST}" ${CXX_TEST_PATH}/${KIT}CxxTests ${TEST})
ENDFOREACH()
