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
set(density_estimate_headers
  ContinuousScatterPlot.h
  Entropy.h
  Histogram.h
  NDEntropy.h
  NDHistogram.h
  ParticleDensityBase.h
  ParticleDensityCloudInCell.h
  ParticleDensityNearestGridPoint.h
  Statistics.h
  )

set(density_estimate_sources_device
  ContinuousScatterPlot.cxx
  Entropy.cxx
  Histogram.cxx
  NDEntropy.cxx
  NDHistogram.cxx
  ParticleDensityBase.cxx
  ParticleDensityCloudInCell.cxx
  ParticleDensityNearestGridPoint.cxx
  Statistics.cxx
  )

viskores_library(
  NAME viskores_filter_density_estimate
  HEADERS ${density_estimate_headers}
  DEVICE_SOURCES ${density_estimate_sources_device}
  USE_VISKORES_JOB_POOL
)

set_property(TARGET
  viskores_filter_density_estimate
  PROPERTY UNITY_BUILD_MODE GROUP
  )

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_density_estimate)

add_subdirectory(worklet)
