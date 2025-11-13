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

set(flow_headers
  FilterParticleAdvection.h
  FilterParticleAdvectionSteadyState.h
  FilterParticleAdvectionUnsteadyState.h
  FlowTypes.h
  Lagrangian.h
  LagrangianStructures.h
  ParticleAdvection.h
  Pathline.h
  PathParticle.h
  Streamline.h
  StreamSurface.h
  WarpXStreamline.h
  )

set(flow_sources
  FilterParticleAdvection.cxx
  internal/BoundsMap.cxx
  )

set(flow_device_sources
  worklet/Analysis.cxx
  Lagrangian.cxx
  LagrangianStructures.cxx
  StreamSurface.cxx
  Lagrangian.cxx
  LagrangianStructures.cxx
  ParticleAdvection.cxx
  Pathline.cxx
  PathParticle.cxx
  Streamline.cxx
  WarpXStreamline.cxx
  )

viskores_add_instantiations(advection_worklet_instantiations
  INSTANTIATIONS_FILE worklet/ParticleAdvection.h
  TEMPLATE_SOURCE worklet/ParticleAdvectionWorklets.h
  )

viskores_library(
  NAME viskores_filter_flow
  HEADERS ${flow_headers}
  SOURCES ${flow_sources}
  DEVICE_SOURCES ${flow_device_sources} ${advection_worklet_instantiations}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_flow)

add_subdirectory(internal)
add_subdirectory(worklet)
