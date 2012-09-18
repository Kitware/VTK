create_test_sourcelist(Tests ${vtk-module}CxxTests.cxx
  TestArrayAPI.cxx
  TestArrayAPIConvenience.cxx
  TestArrayAPIDense.cxx
  TestArrayAPISparse.cxx
  TestArrayBool.cxx
  # TestArrayCasting.cxx # Uses Boost in its own separate test.
  TestArrayExtents.cxx
  TestArrayInterpolationDense.cxx
  TestArrayLookup.cxx
  TestArrayNullValues.cxx
  TestArraySize.cxx
  TestArrayUserTypes.cxx
  TestArrayVariants.cxx
  TestCollection.cxx
  TestConditionVariable.cxx
  # TestCxxFeatures.cxx # This is in its own exe too.
  TestDataArray.cxx
  TestDataArrayComponentNames.cxx
  TestGarbageCollector.cxx
  # TestInstantiator.cxx # Have not enabled instantiators.
  TestMath.cxx
  TestMinimalStandardRandomSequence.cxx
  TestNew.cxx
  TestObjectFactory.cxx
  TestObservers.cxx
  TestObserversPerformance.cxx
  TestSmartPointer.cxx
  TestSortDataArray.cxx
  TestSparseArrayValidation.cxx
  TestSystemInformation.cxx
  TestTimePointUtility.cxx
  TestUnicodeStringAPI.cxx
  TestUnicodeStringArrayAPI.cxx
  TestVariantComparison.cxx
  TestWeakPointer.cxx
  otherArrays.cxx
  otherByteSwap.cxx
  # These two need vtkLookupTableWithEnabling/vtkLogLookupTable - move
  # them to RenderingCore or somewhere out there.
  #otherLookupTable.cxx
  #otherLookupTableWithEnabling.cxx
  otherStringArray.cxx

  EXTRA_INCLUDE vtkTestDriver.h
)

vtk_module_test_executable(${vtk-module}CxxTests ${Tests}
  vtkTestNewVar.cxx)

set(TestsToRun ${Tests})
list(REMOVE_ITEM TestsToRun ${vtk-module}CxxTests.cxx)

# Tell TestSystemInformation where to find the build trees.
set(TestSystemInformation_ARGS ${CMAKE_BINARY_DIR})

# Add all the executables
foreach(test ${TestsToRun})
  get_filename_component(TName ${test} NAME_WE)
  add_test(NAME ${vtk-module}-${TName} COMMAND ${vtk-module}CxxTests ${TName} ${${TName}_ARGS})
endforeach()
