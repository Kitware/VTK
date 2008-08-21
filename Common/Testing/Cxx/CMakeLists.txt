SET(KIT Common)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  ObjectFactory.cxx
  otherArrays.cxx
  otherTimerLog.cxx
  otherLookupTable.cxx
  otherByteSwap.cxx
  otherStringArray.cxx
  TestAmoebaMinimizer.cxx
  TestArrayLookup.cxx
  TestGarbageCollector.cxx
  TestDataArray.cxx
  TestDirectory.cxx
  TestMath.cxx
  TestPolynomialSolversUnivariate.cxx
  TestSmartPointer.cxx
  TestSortDataArray.cxx
  SystemInformation.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests})
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkCommon)

SET (TestsToRun ${Tests})
REMOVE (TestsToRun ${KIT}CxxTests.cxx)

# Configure a header needed by SystemInformation.
CONFIGURE_FILE("${VTK_SOURCE_DIR}/Common/Testing/Cxx/SystemInformation.h.in"
               "${VTK_BINARY_DIR}/Common/Testing/Cxx/SystemInformation.h"
               @ONLY IMMEDIATE)

#
# Add all the executables
FOREACH (test ${TestsToRun})
  GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
  ADD_TEST(${TName} ${CXX_TEST_PATH}/${KIT}CxxTests ${TName})
ENDFOREACH (test)

#
# Add the TestCxxFeatures test by itself so that if it doesn't compile
# we don't loose all the tests for common
#
ADD_EXECUTABLE(TestCxxFeatures TestCxxFeatures.cxx)
TARGET_LINK_LIBRARIES(TestCxxFeatures vtkCommon)
ADD_TEST(TestCxxFeatures ${CXX_TEST_PATH}/TestCxxFeatures)

#
# Add the TestInstantiator test by itself because it is designed to
# test pulling in all class's symbols.  We don't want the other tests
# to draw in symbols.
#
ADD_EXECUTABLE(TestInstantiator TestInstantiator.cxx)
TARGET_LINK_LIBRARIES(TestInstantiator vtkCommon)
ADD_TEST(TestInstantiator ${CXX_TEST_PATH}/TestInstantiator)

#
# Add a test that spits out the cygwin installation info if building
# with cygwin:
#
IF(CYGWIN)
  ADD_TEST(SystemInformation-cygcheck
    "${CMAKE_CURRENT_SOURCE_DIR}/SystemInformation-cygcheck.sh")
ENDIF(CYGWIN)
