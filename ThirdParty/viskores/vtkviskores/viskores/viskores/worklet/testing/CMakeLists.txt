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
  GenerateTestDataSets.h
  )

set(unit_tests
  UnitTestAverageByKey.cxx
  UnitTestBoundingIntervalHierarchy.cxx
  UnitTestCellDeepCopy.cxx
  UnitTestCosmoTools.cxx
  UnitTestDescriptiveStatistics.cxx
  UnitTestDispatcherBase.cxx
  UnitTestFieldStatistics.cxx
  UnitTestKeys.cxx
  UnitTestMaskIndices.cxx
  UnitTestMaskSelect.cxx
  UnitTestNormalize.cxx
  UnitTestNDimsHistMarginalization.cxx
  UnitTestScalarsToColors.cxx
  UnitTestScatterAndMask.cxx
  UnitTestScatterCounting.cxx
  UnitTestScatterPermutation.cxx
  UnitTestSplatKernels.cxx
  UnitTestScatterAndMaskWithTopology.cxx
  UnitTestStreamLineUniformGrid.cxx
  UnitTestTriangleWinding.cxx
  UnitTestWholeCellSetIn.cxx
  UnitTestWorkletCellNeighborhood.cxx
  UnitTestWorkletMapField.cxx
  UnitTestWorkletMapField3d.cxx
  UnitTestWorkletMapFieldExecArg.cxx
  UnitTestWorkletMapFieldWholeArray.cxx
  UnitTestWorkletMapFieldWholeArrayAtomic.cxx
  UnitTestWorkletMapTopologyExplicit.cxx
  UnitTestWorkletMapTopologyUniform.cxx
  UnitTestWorkletPointNeighborhood.cxx
  UnitTestWorkletReduceByKey.cxx
  UnitTestWaveletCompressor.cxx
 )

viskores_unit_tests(
  DEVICE_SOURCES ${unit_tests}
  LIBRARIES viskores_source viskores_worklet viskores_filter viskores_io
  USE_VISKORES_JOB_POOL
  )
if (TARGET viskores_cuda)
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND
     CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 10.0.0)
    set(problematic_cuda_srcs
      "${CMAKE_CURRENT_BINARY_DIR}/UnitTestParticleAdvection.cu"
      "${CMAKE_CURRENT_BINARY_DIR}/UnitTestBoundingIntervalHierarchy.cu"
    )
    set_source_files_properties(${problematic_cuda_srcs} PROPERTIES COMPILE_FLAGS "-Xptxas --opt-level=0")
  endif()
endif()
