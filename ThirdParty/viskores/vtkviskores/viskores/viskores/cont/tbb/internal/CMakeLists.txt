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
  DeviceAdapterAlgorithmTBB.h
  DeviceAdapterMemoryManagerTBB.h
  DeviceAdapterRuntimeDetectorTBB.h
  DeviceAdapterTagTBB.h
  FunctorsTBB.h
  ParallelSortTBB.h
  RuntimeDeviceConfigurationTBB.h
  )

viskores_declare_headers(${headers}
   ParallelSortTBB.hxx
   )

#These sources need to always be built
target_sources(viskores_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorTBB.cxx
  )

#-----------------------------------------------------------------------------
if (TARGET viskores_tbb)
  target_sources(viskores_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmTBB.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ParallelSortTBB.cxx
    )
endif()
