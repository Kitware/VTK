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
set(vector_analysis_headers
  CrossProduct.h
  DotProduct.h
  Gradient.h
  SurfaceNormals.h
  VectorMagnitude.h
  )
set(vector_analysis_sources_device
  CrossProduct.cxx
  DotProduct.cxx
  Gradient.cxx
  SurfaceNormals.cxx
  VectorMagnitude.cxx
  )

viskores_add_instantiations(instantiation_files
  INSTANTIATIONS_FILE worklet/Gradient.h
  )
list(APPEND vector_analysis_sources_device ${instantiation_files})

viskores_library(
  NAME viskores_filter_vector_analysis
  HEADERS ${vector_analysis_headers}
  DEVICE_SOURCES ${vector_analysis_sources_device}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_vector_analysis)

add_subdirectory(worklet)
