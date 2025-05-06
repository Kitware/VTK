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
  Algorithm.h
  ArrayCopy.h
  ArrayCopyDevice.h
  ArrayExtractComponent.h
  ArrayGetValues.h
  ArrayHandle.h
  ArrayHandleBasic.h
  ArrayHandleBitField.h
  ArrayHandleCartesianProduct.h
  ArrayHandleCast.h
  ArrayHandleCompositeVector.h
  ArrayHandleConcatenate.h
  ArrayHandleConstant.h
  ArrayHandleCounting.h
  ArrayHandleDecorator.h
  ArrayHandleDiscard.h
  ArrayHandleExtractComponent.h
  ArrayHandleGroupVec.h
  ArrayHandleGroupVecVariable.h
  ArrayHandleImplicit.h
  ArrayHandleIndex.h
  ArrayHandleMultiplexer.h
  ArrayHandleOffsetsToNumComponents.h
  ArrayHandlePermutation.h
  ArrayHandleRecombineVec.h
  ArrayHandleReverse.h
  ArrayHandleRandomStandardNormal.h
  ArrayHandleRandomUniformBits.h
  ArrayHandleRandomUniformReal.h
  ArrayHandleRuntimeVec.h
  ArrayHandleSOA.h
  ArrayHandleStride.h
  ArrayHandleSwizzle.h
  ArrayHandleTransform.h
  ArrayHandleUniformPointCoordinates.h
  ArrayHandleView.h
  ArrayHandleXGCCoordinates.h
  ArrayHandleZip.h
  ArrayPortal.h
  ArrayPortalToIterators.h
  ArrayRangeCompute.h
  ArrayRangeComputeTemplate.h
  ArrayRangeComputeTemplateInstantiationsIncludes.h
  ArraySetValues.h
  AssignerPartitionedDataSet.h
  AtomicArray.h
  BitField.h
  BoundsCompute.h
  BoundsGlobalCompute.h
  CastAndCall.h
  CellLocatorBase.h
  CellLocatorBoundingIntervalHierarchy.h
  CellLocatorChooser.h
  CellLocatorGeneral.h
  CellLocatorPartitioned.h
  CellLocatorRectilinearGrid.h
  CellLocatorTwoLevel.h
  CellLocatorUniformBins.h
  CellLocatorUniformGrid.h
  CellSet.h
  CellSetExplicit.h
  CellSetExtrude.h
  CellSetList.h
  CellSetPermutation.h
  CellSetSingleType.h
  CellSetStructured.h
  ColorTable.h
  ColorTableMap.h
  ColorTableSamples.h
  ConvertNumComponentsToOffsets.h
  CoordinateSystem.h
  DataSet.h
  DataSetBuilderCurvilinear.h
  DataSetBuilderExplicit.h
  DataSetBuilderRectilinear.h
  DataSetBuilderUniform.h
  DeviceAdapter.h
  DeviceAdapterAlgorithm.h
  DeviceAdapterList.h
  DeviceAdapterTag.h
  DIYMemoryManagement.h
  EnvironmentTracker.h
  Error.h
  ErrorBadAllocation.h
  ErrorBadDevice.h
  ErrorBadType.h
  ErrorBadValue.h
  ErrorExecution.h
  ErrorFilterExecution.h
  ErrorInternal.h
  ErrorUserAbort.h
  ExecutionAndControlObjectBase.h
  ExecutionObjectBase.h
  Field.h
  FieldRangeCompute.h
  FieldRangeGlobalCompute.h
  Initialize.h
  Invoker.h
  Logging.h
  MergePartitionedDataSet.h
  ParticleArrayCopy.h
  PartitionedDataSet.h
  PointLocatorBase.h
  PointLocatorSparseGrid.h
  RuntimeDeviceInformation.h
  RuntimeDeviceTracker.h
  Serialization.h
  Storage.h
  StorageList.h
  Timer.h
  Token.h
  TryExecute.h
  SerializableTypeString.h
  UncertainArrayHandle.h
  UncertainCellSet.h
  UnknownArrayHandle.h
  UnknownCellSet.h
  Variant.h
  )

set(template_sources
  CellSetExplicit.hxx
  ParticleArrayCopy.hxx
  )

set(sources
  ArrayHandle.cxx
  ArrayHandleBasic.cxx
  ArrayHandleSOA.cxx
  ArrayHandleStride.cxx
  AssignerPartitionedDataSet.cxx
  BitField.cxx
  BoundsCompute.cxx
  BoundsGlobalCompute.cxx
  CellLocatorBase.cxx
  CellLocatorGeneral.cxx
  CellLocatorPartitioned.cxx
  CellLocatorRectilinearGrid.cxx
  CellLocatorUniformBins.cxx
  CellLocatorUniformGrid.cxx
  CellSet.cxx
  CellSetStructured.cxx
  ColorTablePresets.cxx
  CoordinateSystem.cxx
  DataSet.cxx
  DataSetBuilderCurvilinear.cxx
  DataSetBuilderExplicit.cxx
  DataSetBuilderRectilinear.cxx
  DataSetBuilderUniform.cxx
  DeviceAdapterTag.cxx
  DIYMemoryManagement.cxx
  EnvironmentTracker.cxx
  ErrorBadDevice.cxx
  ErrorBadType.cxx
  FieldRangeCompute.cxx
  FieldRangeGlobalCompute.cxx
  internal/DeviceAdapterMemoryManager.cxx
  internal/DeviceAdapterMemoryManagerShared.cxx
  internal/FieldCollection.cxx
  internal/RuntimeDeviceConfiguration.cxx
  internal/RuntimeDeviceConfigurationOptions.cxx
  internal/RuntimeDeviceOption.cxx
  Initialize.cxx
  Logging.cxx
  RuntimeDeviceTracker.cxx
  PartitionedDataSet.cxx
  PointLocatorBase.cxx
  Storage.cxx
  Token.cxx
  TryExecute.cxx
  UnknownArrayHandle.cxx
  UnknownCellSet.cxx
  )

# This list of sources has code that uses devices and so might need to be
# compiled with a device-specific compiler (like CUDA).
set(device_sources
  ArrayCopy.cxx
  ArrayGetValues.cxx
  ArrayHandleIndex.cxx
  ArrayHandleUniformPointCoordinates.cxx
  ArrayRangeCompute.cxx
  ArraySetValues.cxx
  CellLocatorBoundingIntervalHierarchy.cxx
  CellLocatorUniformBins.cxx
  CellLocatorTwoLevel.cxx
  CellSetExplicit.cxx
  CellSetExtrude.cxx
  ColorTable.cxx
  ConvertNumComponentsToOffsets.cxx
  Field.cxx
  internal/ArrayCopyUnknown.cxx
  internal/ArrayRangeComputeUtils.cxx
  internal/Buffer.cxx
  internal/MapArrayPermutation.cxx
  MergePartitionedDataSet.cxx
  PointLocatorSparseGrid.cxx
  RuntimeDeviceInformation.cxx
  Timer.cxx
  )

#-----------------------------------------------------------------------------
# Set up default types, which can be custom configured by other build systems.
viskores_get_kit_name(kit_name kit_dir)

# Some custom types included with Viskores
viskores_option(Viskores_USE_DEFAULT_TYPES_FOR_VTK
  "Compile Viskores algorithms for use with types from VTK."
  OFF
  )
if (Viskores_USE_DEFAULT_TYPES_FOR_VTK)
  set(Viskores_DEFAULT_TYPES_HEADER "internal/DefaultTypesVTK.h.in")
  viskores_option(Viskores_ADD_XGC_TO_DEFAULT_TYPES_VTK "Add XGC types to default VTK types." OFF)
  mark_as_advanced(Viskores_ADD_XGC_TO_DEFAULT_TYPES_VTK)
  if (Viskores_ADD_XGC_TO_DEFAULT_TYPES_VTK)
    set(VISKORES_ADD_XGC_DEFAULT_TYPES TRUE)
  endif()
endif()

viskores_option(Viskores_USE_DEFAULT_TYPES_FOR_ASCENT
  "Compile Viskores algorithms for use with types from Ascent."
  OFF
  )
if (Viskores_USE_DEFAULT_TYPES_FOR_ASCENT)
  set(Viskores_DEFAULT_TYPES_HEADER "internal/DefaultTypesAscent.h.in")
  if (Viskores_USE_64BIT_IDS)
    message(SEND_ERROR
      "When Viskores_USE_DEFAULT_TYPES_FOR_ASCENT is ON, Viskores_USE_64BIT_IDS must be set to OFF"
      )
  endif()
  if (NOT Viskores_USE_DOUBLE_PRECISION)
    message(SEND_ERROR
      "When Viskores_USE_DEFAULT_TYPES_FOR_ASCENT is ON, Viskores_USE_DOUBLE_PRECISION must be set to ON"
      )
  endif()
endif()

mark_as_advanced(
  Viskores_USE_DEFAULT_TYPES_FOR_VTK
  Viskores_USE_DEFAULT_TYPES_FOR_ASCENT
  )

if (Viskores_DEFAULT_TYPES_HEADER)
  set(VISKORES_HAS_DEFAULT_TYPES_HEADER TRUE)
  get_filename_component(Viskores_DEFAULT_TYPES_HEADER_FILENAME ${Viskores_DEFAULT_TYPES_HEADER} NAME_WE)
  set(Viskores_DEFAULT_TYPES_HEADER_FILENAME "${Viskores_DEFAULT_TYPES_HEADER_FILENAME}.h")
  configure_file(${Viskores_DEFAULT_TYPES_HEADER}
    ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/internal/${Viskores_DEFAULT_TYPES_HEADER_FILENAME}
    @ONLY
    )
  viskores_install_headers(viskores/cont/internal
    ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/internal/${Viskores_DEFAULT_TYPES_HEADER_FILENAME}
    )
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/DefaultTypes.h.in
  ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/DefaultTypes.h
  @ONLY
  )
viskores_install_headers(viskores/cont
  ${Viskores_BINARY_INCLUDE_DIR}/${kit_dir}/DefaultTypes.h
  )

#-----------------------------------------------------------------------------
# Some operations are pre-compiled for many different types. Improve parallel
# compiles by breaking them up into smaller units.
viskores_add_instantiations(array_range_instantiations
  INSTANTIATIONS_FILE ArrayRangeComputeTemplate.h
  TEMPLATE_SOURCE ArrayRangeComputeTemplateInstantiationsIncludes.h
  )
list(APPEND device_sources ${array_range_instantiations})

#-----------------------------------------------------------------------------
viskores_library( NAME viskores_cont
              SOURCES ${sources}
              TEMPLATE_SOURCES ${template_sources}
              HEADERS ${headers}
              DEVICE_SOURCES ${device_sources}
            )

target_sources(viskores_cont
  PRIVATE
    internal/Hints.h
)

add_subdirectory(internal)
add_subdirectory(arg)

set(backends )
if(TARGET viskores_tbb)
  list(APPEND backends viskores_tbb)
endif()
if(TARGET viskores_cuda)
  list(APPEND backends viskores_cuda)
endif()
if(TARGET viskores_openmp)
  list(APPEND backends viskores_openmp)
endif()
if(TARGET viskores_kokkos)
  list(APPEND backends viskores_kokkos)
endif()

target_link_libraries(viskores_cont PUBLIC viskores_compiler_flags ${backends})
target_link_libraries(viskores_cont PUBLIC Threads::Threads)
