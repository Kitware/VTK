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

  configure_file(TestDataArrayAPI.cxx.in "${test_source}")

  set(data_array_tests ${data_array_tests} "${test_source}.cxx" PARENT_SCOPE)
endfunction()

# Legacy -- test that the vtkDataArrayTemplate interface still works:
if(NOT VTK_LEGACY_REMOVE)
  add_data_array_test(
    vtkDataArrayTemplate_LEGACY
    vtkDataArrayTemplate.h
    float
    vtkDataArrayTemplate<float>
  )
endif()

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
endforeach()

# Tell TestSystemInformation where to find the build trees.
set(TestSystemInformation_ARGS ${CMAKE_BINARY_DIR})

# Tell TestXMLFileOutputWindow where to write test file
set(TestXMLFileOutputWindow_ARGS ${CMAKE_BINARY_DIR}/Testing/Temporary/XMLFileOutputWindow.txt)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  UnitTestMath.cxx
  TestAngularPeriodicDataArray.cxx
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
  TestArrayInterpolationDense.cxx
  TestArrayLookup.cxx
  TestArrayNullValues.cxx
  TestArraySize.cxx
  TestArrayUniqueValueDetection.cxx
  TestArrayUserTypes.cxx
  TestArrayVariants.cxx
  TestCollection.cxx
  TestConditionVariable.cxx
  # TestCxxFeatures.cxx # This is in its own exe too.
  TestDataArray.cxx
  TestDataArrayComponentNames.cxx
  TestDataArrayIterators.cxx
  TestGarbageCollector.cxx
  TestGenericDataArrayAPI.cxx
  TestInformationKeyLookup.cxx
  # TestInstantiator.cxx # Have not enabled instantiators.
  TestLookupTable.cxx
  TestLookupTableThreaded.cxx
  TestMath.cxx
  TestMersenneTwister.cxx
  TestMinimalStandardRandomSequence.cxx
  TestNew.cxx
  TestObjectFactory.cxx
  TestObservers.cxx
  TestObserversPerformance.cxx
  TestOStreamWrapper.cxx
  TestSMP.cxx
  TestSmartPointer.cxx
  TestSortDataArray.cxx
  TestSparseArrayValidation.cxx
  TestSystemInformation.cxx
  TestTemplateMacro.cxx
  TestTimePointUtility.cxx
  TestUnicodeStringAPI.cxx
  TestUnicodeStringArrayAPI.cxx
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
  )

vtk_test_cxx_executable(${vtk-module}CxxTests tests
  vtkTestNewVar.cxx
  )
