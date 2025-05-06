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
set(connected_components_headers
  CellSetConnectivity.h
  ImageConnectivity.h
  )
set(connected_components_sources_device
  CellSetConnectivity.cxx
  ImageConnectivity.cxx
  )

viskores_library(
  NAME viskores_filter_connected_components
  HEADERS ${connected_components_headers}
  DEVICE_SOURCES ${connected_components_sources_device}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter_connected_components PRIVATE viskores_worklet PUBLIC viskores_filter_core)
target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_connected_components)

add_subdirectory(worklet)
