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
set(scalar_topology_headers
  ContourTreeUniform.h
  ContourTreeUniformAugmented.h
  ContourTreeUniformDistributed.h
  DistributedBranchDecompositionFilter.h
  SelectTopVolumeBranchesFilter.h
  ExtractTopVolumeContoursFilter.h
  )

set(scalar_topology_sources
  internal/BranchDecompositionBlock.cxx
  internal/SelectTopVolumeBranchesBlock.cxx
  internal/ExtractTopVolumeContoursBlock.cxx
  internal/ComputeBlockIndices.cxx
  internal/ComputeDistributedBranchDecompositionFunctor.cxx
  internal/SelectTopVolumeBranchesFunctor.cxx
  internal/UpdateParentBranchFunctor.cxx
  internal/ExchangeBranchEndsFunctor.cxx
  ContourTreeUniform.cxx
  ContourTreeUniformAugmented.cxx
  ContourTreeUniformDistributed.cxx
  DistributedBranchDecompositionFilter.cxx
  SelectTopVolumeBranchesFilter.cxx
  ExtractTopVolumeContoursFilter.cxx
  )

viskores_library(
  NAME viskores_filter_scalar_topology
  HEADERS ${scalar_topology_headers}
  DEVICE_SOURCES ${scalar_topology_sources}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter_scalar_topology PUBLIC viskores_worklet viskores_filter_core)
if (Viskores_ENABLE_MPI)
  target_link_libraries(viskores_filter_scalar_topology PUBLIC MPI::MPI_CXX)
endif ()
target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_scalar_topology)

add_subdirectory(internal)
add_subdirectory(worklet)
