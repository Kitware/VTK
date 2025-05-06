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
