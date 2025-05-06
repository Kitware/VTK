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
set(entity_extraction_headers
  ExternalFaces.h
  ExtractGeometry.h
  ExtractPoints.h
  ExtractStructured.h
  GhostCellRemove.h
  Mask.h
  MaskPoints.h
  Threshold.h
  ThresholdPoints.h
  )
set(entity_extraction_sources_device
  ExternalFaces.cxx
  ExtractGeometry.cxx
  ExtractPoints.cxx
  ExtractStructured.cxx
  GhostCellRemove.cxx
  Mask.cxx
  MaskPoints.cxx
  Threshold.cxx
  ThresholdPoints.cxx
  )

viskores_library(
  NAME viskores_filter_entity_extraction
  HEADERS ${entity_extraction_headers}
  DEVICE_SOURCES ${entity_extraction_sources_device}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_entity_extraction)

add_subdirectory(worklet)
