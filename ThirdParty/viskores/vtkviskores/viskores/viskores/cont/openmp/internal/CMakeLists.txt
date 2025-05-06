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
  DeviceAdapterAlgorithmOpenMP.h
  DeviceAdapterMemoryManagerOpenMP.h
  DeviceAdapterRuntimeDetectorOpenMP.h
  DeviceAdapterTagOpenMP.h
  FunctorsOpenMP.h
  ParallelQuickSortOpenMP.h
  ParallelRadixSortOpenMP.h
  ParallelScanOpenMP.h
  ParallelSortOpenMP.h
  RuntimeDeviceConfigurationOpenMP.h
  )

viskores_declare_headers(${headers})

#These sources need to always be built
target_sources(viskores_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorOpenMP.cxx
  )

#-----------------------------------------------------------------------------
if (TARGET viskores_openmp)
  target_sources(viskores_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmOpenMP.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ParallelRadixSortOpenMP.cxx
    )
endif()
