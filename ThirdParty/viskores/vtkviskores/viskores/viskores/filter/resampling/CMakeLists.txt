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
set(resampling_headers
  Probe.h
  HistSampling.h
  )

set(resampling_sources
  Probe.cxx
  HistSampling.cxx
  )

viskores_library(
  NAME viskores_filter_resampling
  HEADERS ${resampling_headers}
  DEVICE_SOURCES ${resampling_sources}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_resampling)

add_subdirectory(worklet)
