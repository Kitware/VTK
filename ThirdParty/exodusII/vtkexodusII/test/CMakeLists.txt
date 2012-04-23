SET( cbind_TEST_SRCS
  CreateEdgeFace.c
  ReadEdgeFace.c
)

SET_SOURCE_FILES_PROPERTIES(
  CreateEdgeFace.c
  ReadEdgeFace.c
  COMPILE_FLAGS -DEXO_CTEST
)

# Always report error/warning messages during testing:
# The ex_opts call precipitates the EXTRA_INCLUDE below.
SET( CMAKE_TESTDRIVER_BEFORE_TESTMAIN
  "${CMAKE_TESTDRIVER_BEFORE_TESTMAIN}  ex_opts( EX_VERBOSE );\n"
)
CREATE_TEST_SOURCELIST( ExoIICTests
  ExoIICTests.cxx
  ${cbind_TEST_SRCS}
  EXTRA_INCLUDE exodusII_test.h
  )

ADD_EXECUTABLE( ExoIICTests ${ExoIICTests} )
TARGET_LINK_LIBRARIES( ExoIICTests vtkCommon vtkexoIIc vtkNetCDF )

SET( TestsToRun ${ExoIICTests} )
REMOVE( TestsToRun ExoIICTests.cxx )

FOREACH( test ${TestsToRun} )
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST( ${TName} ${EXECUTABLE_OUTPUT_PATH}/ExoIICTests ${TName} )
ENDFOREACH( test )
ADD_TEST( CreateEdgeFaceWithConcats ${EXECUTABLE_OUTPUT_PATH}/ExoIICTests CreateEdgeFace -pcab -pcset -pvpax )
ADD_TEST( ReadEdgeFaceWithConcats ${EXECUTABLE_OUTPUT_PATH}/ExoIICTests ReadEdgeFace )

