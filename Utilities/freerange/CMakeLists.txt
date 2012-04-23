PROJECT(freerange)

CREATE_TEST_SOURCELIST( freerangeTestSrcs freerangeTests.cxx
  testInsertionDeletion.cxx
  testIteration.cxx
  EXTRA_INCLUDE "freerangeTestDriver.h"
)

INCLUDE_DIRECTORIES(
  BEFORE
  ${freerange_SOURCE_DIR}
)

SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
SET( LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

ADD_EXECUTABLE( freerangeTests ${freerangeTestSrcs} )

SET( TestsToRun ${freerangeTestSrcs} )
LIST( REMOVE_ITEM TestsToRun freerangeTests.cxx )

FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CMAKE_CURRENT_BINARY_DIR}/freerangeTests ${TName})
ENDFOREACH (test)

