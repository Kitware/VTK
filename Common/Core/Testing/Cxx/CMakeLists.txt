# Add the vtkDataArrayAPI tests. These are split up to separately test each
# array class, as testing all in a single translation unit led to heavy
# memory consumption and excessively long compile times.
set(data_array_tests)
function(add_data_array_test test_name array_header array_valuetype array_type)
  set(VTK_TESTDATAARRAYAPI_HEADER "${array_header}")
  set(VTK_TESTDATAARRAYAPI_VALUETYPE "${array_valuetype}")
  set(VTK_TESTDATAARRAYAPI_ARRAYTYPE "${array_type}")
  set(VTK_TESTDATAARRAYAPI_TESTNAME "TestDataArrayAPI_${test_name}")
  set(test_source "${VTK_TESTDATAARRAYAPI_TESTNAME}.cxx")

  configure_file(TestDataArrayAPI.cxx.in "${CMAKE_CURRENT_BINARY_DIR}/${test_source}")

  #specify a custom test name as these files exist in the build directory
  list(APPEND data_array_tests "${VTK_TESTDATAARRAYAPI_TESTNAME},${CMAKE_CURRENT_BINARY_DIR}/${test_source}" PARENT_SCOPE)
endfunction()

# Non-templated classes:
add_data_array_test(vtkCharArray vtkCharArray.h "char" vtkCharArray)
add_data_array_test(vtkDoubleArray vtkDoubleArray.h "double" vtkDoubleArray)
add_data_array_test(vtkFloatArray vtkFloatArray.h "float" vtkFloatArray)
add_data_array_test(vtkIntArray vtkIntArray.h "int" vtkIntArray)
add_data_array_test(vtkLongArray vtkLongArray.h "long" vtkLongArray)
add_data_array_test(vtkLongLongArray vtkLongLongArray.h "long long" vtkLongLongArray)
add_data_array_test(vtkShortArray vtkShortArray.h "short" vtkShortArray)
add_data_array_test(vtkSignedCharArray vtkSignedCharArray.h "signed char" vtkSignedCharArray)
add_data_array_test(vtkUnsignedCharArray vtkUnsignedCharArray.h "unsigned char" vtkUnsignedCharArray)
add_data_array_test(vtkUnsignedIntArray vtkUnsignedIntArray.h "unsigned int" vtkUnsignedIntArray)
add_data_array_test(vtkUnsignedLongArray vtkUnsignedLongArray.h "unsigned long" vtkUnsignedLongArray)
add_data_array_test(vtkUnsignedLongLongArray vtkUnsignedLongLongArray.h "unsigned long long" vtkUnsignedLongLongArray)
add_data_array_test(vtkUnsignedShortArray vtkUnsignedShortArray.h "unsigned short" vtkUnsignedShortArray)
add_data_array_test(vtkIdTypeArray vtkIdTypeArray.h "vtkIdType" vtkIdTypeArray)
set(value_types
  char
  double
  float
  int
  long
  "long long"
  short
  "signed char"
  "unsigned char"
  "unsigned int"
  "unsigned long"
  "unsigned long long"
  "unsigned short"
  vtkIdType
)

# Templated data array subclasses can be done in a loop:
foreach(value_type ${value_types})
  string(REPLACE " " "_" pretty_value_type "${value_type}")
  add_data_array_test(
    "AoS_${pretty_value_type}"
    vtkAOSDataArrayTemplate.h
    "${value_type}"
    "vtkAOSDataArrayTemplate<${value_type}>")
  add_data_array_test(
    "SoA_${pretty_value_type}"
    vtkSOADataArrayTemplate.h
    "${value_type}"
    "vtkSOADataArrayTemplate<${value_type}>")

  if(VTK_BUILD_SCALED_SOA_ARRAYS)
    add_data_array_test(
      "ScaleAoS_${pretty_value_type}"
      vtkScaleAOSDataArrayTemplate.h
      "${value_type}"
      "vtkScaleAOSDataArrayTemplate<${value_type}>")
    add_data_array_test(
      "ScaleSoA_${pretty_value_type}"
      vtkScaledSOADataArrayTemplate.h
      "${value_type}"
      "vtkScaledSOADataArrayTemplate<${value_type}>")
  endif()
endforeach()

# Tell TestSystemInformation where to find the build trees.
set(TestSystemInformation_ARGS ${CMAKE_BINARY_DIR})

# Tell TestXMLFileOutputWindow where to write test file
set(TestXMLFileOutputWindow_ARGS ${CMAKE_BINARY_DIR}/Testing/Temporary/XMLFileOutputWindow.txt)

set(TestCLI11_ARGS --file=sample.vtk -c 100 --flag)

set(TestSMP_ARGS
  --Sequential=$<BOOL:${VTK_SMP_ENABLE_SEQUENTIAL}>
  --STDThread=$<BOOL:${VTK_SMP_ENABLE_STDTHREAD}>
  --TBB=$<OR:$<BOOL:${VTK_SMP_ENABLE_TBB}>,$<STREQUAL:"${VTK_SMP_IMPLEMENTATION_TYPE}","TBB">>
  --OpenMP=$<OR:$<BOOL:${VTK_SMP_ENABLE_OPENMP}>,$<STREQUAL:"${VTK_SMP_IMPLEMENTATION_TYPE}","OpenMP">>)

if (VTK_BUILD_SCALED_SOA_ARRAYS)
  set(scale_soa_test TestScaledSOADataArrayTemplate.cxx)
endif ()


vtk_add_test_cxx(vtkCommonCoreCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ExampleDataArrayRangeAPI.cxx
  ExampleDataArrayRangeDispatch.cxx
  UnitTestMath.cxx
  TestAbstractArraySize.cxx
  TestArrayAPI.cxx
  TestArrayAPIConvenience.cxx
  TestArrayAPIDense.cxx
  TestArrayAPISparse.cxx
  TestArrayBool.cxx
  TestArrayDispatchers.cxx
  TestAtomic.cxx
  TestScalarsToColors.cxx
  # TestArrayCasting.cxx # Uses Boost in its own separate test.
  TestArrayExtents.cxx
  TestArrayFreeFunctions.cxx
  TestArrayInterpolationDense.cxx
  TestArrayLookup.cxx
  TestArrayNullValues.cxx
  TestArraySize.cxx
  TestArrayUniqueValueDetection.cxx
  TestArrayUserTypes.cxx
  TestArrayVariants.cxx
  TestBitArray.cxx
  TestCLI11.cxx
  TestCollection.cxx
  # TestCxxFeatures.cxx # This is in its own exe too.
  TestDataArray.cxx
  TestDataArrayComponentNames.cxx
  TestDataArrayIterators.cxx
  TestDataArraySelection.cxx
  TestDataArrayTupleRange.cxx
  TestDataArrayValueRange.cxx
  TestFMT.cxx
  TestGarbageCollector.cxx
  TestGenericDataArrayAPI.cxx
  TestInformationKeyLookup.cxx
  TestLogger.cxx
  TestLoggerThreadName.cxx
  TestLookupTable.cxx
  TestLookupTableThreaded.cxx
  TestMath.cxx
  TestMersenneTwister.cxx
  TestMinimalStandardRandomSequence.cxx
  TestNew.cxx
  TestNumberOfGenerationsFromBase.cxx
  TestObjectFactory.cxx
  TestObservers.cxx
  TestObserversPerformance.cxx
  TestOStreamWrapper.cxx
  TestSMP.cxx
  TestSmartPointer.cxx
  TestSOADataArray.cxx
  TestSortDataArray.cxx
  TestSparseArrayValidation.cxx
  TestSystemInformation.cxx
  TestTemplateMacro.cxx
  TestTimePointUtility.cxx
  TestVariant.cxx
  TestVariantComparison.cxx
  TestWeakPointer.cxx
  TestXMLFileOutputWindow.cxx
  UnitTestInformationKeys.cxx
  otherArrays.cxx
  otherByteSwap.cxx
  # These two need vtkLookupTableWithEnabling/vtkLogLookupTable - move
  # them to RenderingCore or somewhere out there.
  #otherLookupTable.cxx
  #otherLookupTableWithEnabling.cxx
  otherStringArray.cxx

  ${data_array_tests}

  ${scale_soa_test}
  )

# Special test to check whether disabling loguru signal handling, i.e.,
# stack trace printing when it catches a signal, works. We add an
# executable where the stack trace is disabled. The actual test is a
# CMake script that runs the test executable and analyzes the output to
# determine whether a stack trace was printed or not.
add_executable(TestLoggerDisableSignalHandlerCxx
  TestLoggerDisableSignalHandler.cxx)
target_link_libraries(TestLoggerDisableSignalHandlerCxx
  PRIVATE VTK::CommonCore VTK::CommonDataModel)
add_test(NAME TestLoggerDisableSignalHandler
  COMMAND ${CMAKE_COMMAND}
  -DEXECUTABLE_PATH:FILEPATH=$<TARGET_FILE:TestLoggerDisableSignalHandlerCxx>
  -P ${CMAKE_CURRENT_SOURCE_DIR}/TestLoggerDisableSignalHandler.cmake)

vtk_test_cxx_executable(vtkCommonCoreCxxTests tests
  vtkTestNewVar.cxx
  )
