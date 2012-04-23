SET(KIT Common)
CREATE_TEST_SOURCELIST(Tests ${KIT}CxxTests.cxx
  TestObjectFactory.cxx
  otherArrays.cxx
  otherTimerLog.cxx
  otherLookupTable.cxx
  otherLookupTableWithEnabling.cxx
  otherByteSwap.cxx
  otherStringArray.cxx
  TestAmoebaMinimizer.cxx
  TestArrayLookup.cxx
  TestConditionVariable.cxx
  TestGarbageCollector.cxx
  TestDataArray.cxx
  TestDataArrayComponentNames.cxx
  TestDirectory.cxx
  TestFastNumericConversion.cxx
  TestMath.cxx
  TestMatrix3x3.cxx
  TestMinimalStandardRandomSequence.cxx
  TestNew.cxx
  TestObservers.cxx
  TestPlane.cxx
  TestBoundingBox.cxx
  TestPolynomialSolversUnivariate.cxx
  TestSmartPointer.cxx
  TestSortDataArray.cxx
  TestStructuredData.cxx
  TestUnicodeStringAPI.cxx
  TestUnicodeStringArrayAPI.cxx
  TestVariantComparison.cxx
  TestVector.cxx
  TestVectorOperators.cxx
  TestWeakPointer.cxx
  TestSystemInformation.cxx
  EXTRA_INCLUDE vtkTestDriver.h
)

ADD_EXECUTABLE(${KIT}CxxTests ${Tests} vtkTestNewVar.cxx)
TARGET_LINK_LIBRARIES(${KIT}CxxTests vtkCommon)

SET (TestsToRun ${Tests})
LIST (REMOVE_ITEM TestsToRun ${KIT}CxxTests.cxx)

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
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy 
    ${CMAKE_CURRENT_SOURCE_DIR}/SystemInformation-cygcheck.sh
    ${CMAKE_CURRENT_BINARY_DIR}/SystemInformation-cygcheck.sh)
  EXECUTE_PROCESS(COMMAND dos2unix 
    ${CMAKE_CURRENT_BINARY_DIR}/SystemInformation-cygcheck.sh)
  ADD_TEST(SystemInformation-cygcheck
    "${CMAKE_CURRENT_BINARY_DIR}/SystemInformation-cygcheck.sh")
ENDIF(CYGWIN)

IF(VTK_USE_N_WAY_ARRAYS)

  CREATE_TEST_SOURCELIST(ArrayTests ArrayCxxTests.cxx
    TestArrayAPI.cxx
    TestArrayAPIConvenience.cxx
    TestArrayAPIDense.cxx
    TestArrayAPISparse.cxx
    TestArrayBool.cxx
    TestArrayInterpolationDense.cxx
    TestArrayNullValues.cxx
    TestArrayExtents.cxx
    TestArrayUserTypes.cxx
    TestArrayVariants.cxx
    TestArraySize.cxx
    TestSparseArrayValidation.cxx
    EXTRA_INCLUDE vtkTestDriver.h
    )

  ADD_EXECUTABLE(ArrayCxxTests ${ArrayTests})
  TARGET_LINK_LIBRARIES(ArrayCxxTests vtkCommon)

  SET(ArrayTestsToRun ${ArrayTests})
  LIST(REMOVE_ITEM ArrayTestsToRun ArrayCxxTests.cxx)

  FOREACH(test ${ArrayTestsToRun})
    GET_FILENAME_COMPONENT(TName ${test} NAME_WE)
    ADD_TEST(Array-${TName} ${CXX_TEST_PATH}/ArrayCxxTests ${TName})
  ENDFOREACH(test)

  IF(VTK_USE_BOOST)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_EXECUTABLE(TestArrayCasting TestArrayCasting.cxx)
    TARGET_LINK_LIBRARIES(TestArrayCasting vtkCommon)
    ADD_TEST(Array-TestArrayCasting ${CXX_TEST_PATH}/TestArrayCasting)
  ENDIF()

ENDIF(VTK_USE_N_WAY_ARRAYS)

# TestConditionVariable will attempt to allocate a thread for each available core
# and thus should not be run in parallel.
set_tests_properties(TestConditionVariable PROPERTIES RUN_SERIAL ON)
