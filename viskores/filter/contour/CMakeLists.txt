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

set(contour_headers
  AbstractContour.h
  ClipWithField.h
  ClipWithImplicitFunction.h
  Contour.h
  ContourDimension.h
  ContourFlyingEdges.h
  ContourMarchingCells.h
  MIRFilter.h
  Slice.h
  SliceMultiple.h
)

set(contour_sources_device
  ClipWithField.cxx
  ClipWithImplicitFunction.cxx
  ContourFlyingEdges.cxx
  ContourMarchingCells.cxx
  MIRFilter.cxx
  Slice.cxx
  SliceMultiple.cxx
)

set(contour_sources
  # Contour defers worklet compilation to other filters,
  # so it does not need to be compiled with a device adapter.
  Contour.cxx
)

viskores_add_instantiations(contour_instantiations INSTANTIATIONS_FILE worklet/ContourMarchingCells.h)
list(APPEND contour_sources_device ${contour_instantiations})

set_source_files_properties(Contour.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)

viskores_library(
  NAME viskores_filter_contour
  SOURCES ${contour_sources}
  HEADERS ${contour_headers}
  DEVICE_SOURCES ${contour_sources_device}
  USE_VISKORES_JOB_POOL
)

set_property(TARGET
  viskores_filter_contour
  PROPERTY UNITY_BUILD_MODE GROUP
)

if (Viskores_ENABLE_MPI)
  target_link_libraries(viskores_filter_contour PUBLIC MPI::MPI_CXX)
endif ()

target_link_libraries(viskores_filter PUBLIC INTERFACE
   viskores_filter_contour
)

add_subdirectory(worklet)
