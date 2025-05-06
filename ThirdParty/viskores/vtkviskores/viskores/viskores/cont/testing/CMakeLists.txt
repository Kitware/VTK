##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  TestingDeviceAdapter.h
  TestingRuntimeDeviceConfiguration.h
  TestingSerialization.h
  )

set(unit_tests
  UnitTestArrayExtractComponent.cxx
  UnitTestArrayGetValues.cxx
  UnitTestArrayHandleCartesianProduct.cxx
  UnitTestArrayHandleCompositeVector.cxx
  UnitTestArrayHandleCounting.cxx
  UnitTestArrayHandleDiscard.cxx
  UnitTestArrayHandleIndex.cxx
  UnitTestArrayHandleOffsetsToNumComponents.cxx
  UnitTestArrayHandleRandomUniformBits.cxx
  UnitTestArrayHandleReverse.cxx
  UnitTestArrayHandleThreadSafety.cxx
  UnitTestArrayHandleUniformPointCoordinates.cxx
  UnitTestArrayPortalFromIterators.cxx
  UnitTestArrayPortalToIterators.cxx
  UnitTestArraySetValues.cxx
  UnitTestBuffer.cxx
  UnitTestComputeRange.cxx
  UnitTestControlSignatureTag.cxx
  UnitTestContTesting.cxx
  UnitTestDataSetBuilderCurvilinear.cxx
  UnitTestDataSetBuilderExplicit.cxx
  UnitTestDataSetBuilderRectilinear.cxx
  UnitTestDataSetBuilderUniform.cxx
  UnitTestDataSetConvertToExpected.cxx
  UnitTestDataSetExplicit.cxx
  UnitTestDataSetRectilinear.cxx
  UnitTestDataSetUniform.cxx
  UnitTestDeviceAdapterAlgorithmGeneral.cxx
  UnitTestDeviceSelectOnThreads.cxx
  UnitTestError.cxx
  UnitTestFieldRangeCompute.cxx
  UnitTestInitialize.cxx
  UnitTestIteratorFromArrayPortal.cxx
  UnitTestLateDeallocate.cxx
  UnitTestLogging.cxx
  UnitTestMergePartitionedDataSet.cxx
  UnitTestMoveConstructors.cxx
  UnitTestPartitionedDataSet.cxx
  UnitTestRuntimeConfigurationOptions.cxx
  UnitTestRuntimeDeviceInformation.cxx
  UnitTestRuntimeDeviceNames.cxx
  UnitTestScopedRuntimeDeviceTracker.cxx
  UnitTestStorageList.cxx
  UnitTestTimer.cxx
  UnitTestToken.cxx
  UnitTestTryExecute.cxx
  UnitTestTypeCheckArray.cxx
  UnitTestTypeCheckCellSet.cxx
  UnitTestTypeCheckExecObject.cxx
  UnitTestTypeCheckKeys.cxx
  UnitTestUnknownArrayHandle.cxx
  UnitTestUnknownCellSet.cxx
  )

set(unit_tests_device
  UnitTestAlgorithm.cxx
  UnitTestArrayCopy.cxx
  UnitTestArrayHandle.cxx
  UnitTestArrayHandleConcatenate.cxx
  UnitTestArrayHandleConstant.cxx
  UnitTestArrayHandleCast.cxx
  UnitTestArrayHandleDecorator.cxx
  UnitTestArrayHandleExtractComponent.cxx
  UnitTestArrayHandleGroupVec.cxx
  UnitTestArrayHandleGroupVecVariable.cxx
  UnitTestArrayHandleImplicit.cxx
  UnitTestArrayHandleMultiplexer.cxx
  UnitTestArrayHandlePermutation.cxx
  UnitTestArrayHandleRandomStandardNormal.cxx
  UnitTestArrayHandleRandomUniformReal.cxx
  UnitTestArrayHandleRecombineVec.cxx
  UnitTestArrayHandleRuntimeVec.cxx
  UnitTestArrayHandleSOA.cxx
  UnitTestArrayHandleSwizzle.cxx
  UnitTestArrayHandleTransform.cxx
  UnitTestArrayHandleView.cxx
  UnitTestArrayHandleXGCCoordinates.cxx
  UnitTestArrayHandleZip.cxx
  UnitTestArrayRangeCompute.cxx
  UnitTestBitField.cxx
  UnitTestCellLocatorChooser.cxx
  UnitTestCellLocatorGeneral.cxx
  UnitTestCellLocatorPartitioned.cxx
  UnitTestCellLocatorRectilinearGrid.cxx
  UnitTestCellLocatorUniformGrid.cxx
  UnitTestCellLocatorUnstructured.cxx
  UnitTestCellSet.cxx
  UnitTestCellSetExplicit.cxx
  UnitTestCellSetPermutation.cxx
  UnitTestColorTable.cxx
  UnitTestDataSetPermutation.cxx
  UnitTestDataSetSingleType.cxx
  UnitTestDeviceAdapterAlgorithmDependency.cxx
  UnitTestHints.cxx
  UnitTestImplicitFunction.cxx
  UnitTestParticleArrayCopy.cxx
  UnitTestPointLocatorSparseGrid.cxx
  UnitTestTransportArrayIn.cxx
  UnitTestTransportArrayInOut.cxx
  UnitTestTransportArrayOut.cxx
  UnitTestTransportCellSetIn.cxx
  UnitTestTransportExecObject.cxx
  UnitTestTransportWholeArray.cxx
  )
if(TARGET viskores_filter_field_conversion)
  list(APPEND unit_tests_device
    UnitTestCellSetExtrude.cxx
    )
endif()

if(TARGET viskores_filter_contour)
  list(APPEND unit_tests
    UnitTestAbort.cxx
  )
endif()

viskores_unit_tests(SOURCES ${unit_tests} DEVICE_SOURCES ${unit_tests_device})

target_sources(UnitTests_viskores_cont_testing
  PRIVATE
    UnitTestHints.cxx
)

#add distributed tests i.e.test to run with MPI
#if MPI is enabled.
set(mpi_unit_tests
  UnitTestFieldRangeGlobalCompute.cxx
  UnitTestSerializationArrayHandle.cxx
  UnitTestSerializationDataSet.cxx
  )
viskores_unit_tests(MPI SOURCES ${mpi_unit_tests})
