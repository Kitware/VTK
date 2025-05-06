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
set(field_transform_headers
  CompositeVectors.h
  CylindricalCoordinateTransform.h
  FieldToColors.h
  GenerateIds.h
  LogValues.h
  PointElevation.h
  PointTransform.h
  SphericalCoordinateTransform.h
  Warp.h
  WarpScalar.h
  WarpVector.h
  )

set(field_transform_sources
  CylindricalCoordinateTransform.cxx
  FieldToColors.cxx
  GenerateIds.cxx
  LogValues.cxx
  PointElevation.cxx
  PointTransform.cxx
  SphericalCoordinateTransform.cxx
  Warp.cxx
  )

set(field_transform_sources_no_device
  CompositeVectors.cxx
  )

viskores_library(
  NAME viskores_filter_field_transform
  HEADERS ${field_transform_headers}
  DEVICE_SOURCES ${field_transform_sources}
  SOURCES ${field_transform_sources_no_device}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter_field_transform PUBLIC viskores_worklet viskores_filter_core)
target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_field_transform)

add_subdirectory(worklet)
