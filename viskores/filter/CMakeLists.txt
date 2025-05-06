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

set(core_headers
  FieldSelection.h
  Filter.h
  FilterField.h #deprecated
  MapFieldMergeAverage.h
  MapFieldPermutation.h
  TaskQueue.h
  )
set(core_sources
  FieldSelection.cxx
  )
set(core_sources_device
  MapFieldMergeAverage.cxx
  MapFieldPermutation.cxx
  Filter.cxx
  )

viskores_library(
  NAME viskores_filter_core
  HEADERS ${core_headers}
  SOURCES ${core_sources}
  DEVICE_SOURCES ${core_sources_device}
  USE_VISKORES_JOB_POOL
)

add_library(viskores_filter INTERFACE)

set_target_properties(
  viskores_filter_core
  PROPERTIES
  UNITY_BUILD ON
  UNITY_BUILD_MODE BATCH
)

target_link_libraries(viskores_filter PUBLIC INTERFACE
  viskores_filter_core
  )

viskores_install_targets(TARGETS viskores_filter)
