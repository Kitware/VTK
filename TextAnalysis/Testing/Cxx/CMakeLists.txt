# List tests that can be built and run automatically ...
SET(AUTO_TESTS
  TestAssignMimeType
  TestConcatenateArray
  TestDocumentReader
  TestExtractSelectedSlices
  TestMatrixWeighting
  TestMimeTypes
  TestNGramExtraction
  TestPointwiseMutualInformation
  TestPowerWeighting
  TestScaleDimension
  TestSelectArraySlices
  TestTermDictionary
  TestTextExtraction
  TestTokenLengthFilter
  TestTokenValueFilter
  TestTokenizer
  TestTokenizerRanges
  )

SET(AUTO_TEST_LIBRARIES
  vtkCommon
  vtkInfovis
  vtkTextAnalysis
  )

IF(VTK_USE_QT)
  INCLUDE(${QT_USE_FILE})
  LIST(APPEND AUTO_TESTS TestQtXMLProcessor) 

  LIST (APPEND AUTO_TEST_LIBRARIES ${QT_LIBRARIES})
ENDIF()

# Build the auto test executable ...
FOREACH(TEST ${AUTO_TESTS})
  LIST(APPEND TEST_SOURCES ${TEST}.cxx)
ENDFOREACH()

CREATE_TEST_SOURCELIST(TEST_SOURCES ${KIT}CxxTests.cxx 
  ${TEST_SOURCES} 
  EXTRA_INCLUDE vtkTestDriver.h
  )

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/TextAnalysisTestConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/TextAnalysisTestConfig.h)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ADD_EXECUTABLE(${KIT}CxxTests ${TEST_SOURCES})
TARGET_LINK_LIBRARIES(${KIT}CxxTests ${AUTO_TEST_LIBRARIES})

# Setup automatially-run tests ...
FOREACH(TEST ${AUTO_TESTS})
  ADD_TEST("Text-${TEST}" ${CXX_TEST_PATH}/${KIT}CxxTests ${TEST})
ENDFOREACH()

