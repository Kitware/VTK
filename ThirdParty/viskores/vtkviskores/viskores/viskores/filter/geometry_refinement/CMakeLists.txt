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
set(geometry_refinement_headers
  ConvertToPointCloud.h
  Shrink.h
  SplitSharpEdges.h
  Tetrahedralize.h
  Triangulate.h
  Tube.h
  VertexClustering.h
  )

set(geometry_refinement_sources
  ConvertToPointCloud.cxx
  Shrink.cxx
  SplitSharpEdges.cxx
  Tetrahedralize.cxx
  Triangulate.cxx
  Tube.cxx
  VertexClustering.cxx
  )

viskores_library(
  NAME viskores_filter_geometry_refinement
  HEADERS ${geometry_refinement_headers}
  DEVICE_SOURCES ${geometry_refinement_sources}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter_geometry_refinement PUBLIC viskores_worklet viskores_filter_core)
target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_geometry_refinement)

add_subdirectory(worklet)
