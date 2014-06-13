# Tell TestSystemInformation where to find the build trees.
set(TestSystemInformation_ARGS ${CMAKE_BINARY_DIR})

# Tell TestXMLFileOutputWindow where to write test file
set(TestXMLFileOutputWindow_ARGS ${CMAKE_BINARY_DIR}/Testing/Temporary/XMLFileOutputWindow.txt)

vtk_add_test_cxx(${vtk-module}CxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  UnitTestMath.cxx
  TestArrayAPI.cxx
  TestArrayAPIConvenience.cxx
  TestArrayAPIDense.cxx
  TestArrayAPISparse.cxx
  TestArrayBool.cxx
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
  # TestInstantiator.cxx # Have not enabled instantiators.
  TestLookupTable.cxx
  TestMath.cxx
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
  )
vtk_test_cxx_executable(${vtk-module}CxxTests tests
  vtkTestNewVar.cxx
  )
