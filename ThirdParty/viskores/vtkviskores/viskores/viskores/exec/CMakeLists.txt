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

set(headers
  AtomicArrayExecutionObject.h
  BoundaryState.h
  CellDerivative.h
  CellEdge.h
  CellFace.h
  CellInside.h
  CellInterpolate.h
  CellLocatorBoundingIntervalHierarchy.h
  CellLocatorMultiplexer.h
  CellLocatorPartitioned.h
  CellLocatorRectilinearGrid.h
  CellLocatorTwoLevel.h
  CellLocatorUniformBins.h
  CellLocatorUniformGrid.h
  CellMeasure.h
  ColorTable.h
  ConnectivityExplicit.h
  ConnectivityExtrude.h
  ConnectivityPermuted.h
  ConnectivityStructured.h
  FieldNeighborhood.h
  FunctorBase.h
  ParametricCoordinates.h
  PointLocatorSparseGrid.h
  TaskBase.h
  SplineEvaluateRectilinearGrid.h
  SplineEvaluateUniformGrid.h
  Variant.h
  )

set(header_impls
  ColorTable.hxx
  )

#-----------------------------------------------------------------------------
add_subdirectory(internal)
add_subdirectory(arg)

viskores_declare_headers(${headers}
                    ${header_impls}
                    )

set(backends )
if(TARGET viskores_tbb)
  list(APPEND backends viskores_tbb)
endif()
if(TARGET viskores_cuda)
  list(APPEND backends viskores_cuda)
endif()
if(TARGET viskores_openmp)
  list(APPEND backends viskores_openmp)
endif()
if(TARGET viskores_kokkos)
  list(APPEND backends viskores_kokkos)
endif()

add_library(viskores_exec INTERFACE)
target_link_libraries(viskores_exec INTERFACE ${backends})
target_compile_definitions(viskores_exec INTERFACE VISKORES_DEVICE_LINKED)
install(TARGETS viskores_exec EXPORT ${Viskores_EXPORT_NAME})
