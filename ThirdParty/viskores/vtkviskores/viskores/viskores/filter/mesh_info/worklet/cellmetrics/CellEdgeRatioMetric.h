//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_worklet_cellmetrics_CellEdgeRatioMetric_h
#define viskores_worklet_cellmetrics_CellEdgeRatioMetric_h

/*
 * Mesh quality metric functions that compute the edge ratio of mesh cells.
 * The edge ratio of a cell is defined as the length (magnitude) of the longest
 * cell edge divided by the length of the shortest cell edge.
 *
 * These metric computations are adapted from the VTK implementation of the Verdict library,
 * which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
 * of mesh spaces.
 *
 * The edge ratio computations for a pyramid cell types is not defined in the
 * VTK implementation, but is provided here.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
 * See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
 */

#include "viskores/CellShape.h"
#include "viskores/CellTraits.h"
#include "viskores/VecTraits.h"
#include "viskores/VectorAnalysis.h"
#include "viskores/exec/FunctorBase.h"

#define UNUSED(expr) (void)(expr);

namespace viskores
{
namespace worklet
{
namespace cellmetrics
{

using FloatType = viskores::FloatDefault;


template <typename OutType, typename VecType>
VISKORES_EXEC inline OutType ComputeEdgeRatio(const VecType& edges)
{
  const viskores::Id numEdges = edges.GetNumberOfComponents();

  //Compare edge lengths to determine the longest and shortest
  //TODO: Could we use lambda expression here?

  FloatType e0Len = (FloatType)viskores::MagnitudeSquared(edges[0]);
  FloatType currLen, minLen = e0Len, maxLen = e0Len;
  for (int i = 1; i < numEdges; i++)
  {
    currLen = (FloatType)viskores::MagnitudeSquared(edges[i]);
    if (currLen < minLen)
      minLen = currLen;
    if (currLen > maxLen)
      maxLen = currLen;
  }

  if (minLen < viskores::NegativeInfinity<FloatType>())
    return viskores::Infinity<OutType>();

  //Take square root because we only did magnitude squared before
  OutType edgeRatio = (OutType)viskores::Sqrt(maxLen / minLen);
  if (edgeRatio > 0)
    return viskores::Min(edgeRatio, viskores::Infinity<OutType>()); //normal case

  return viskores::Max(edgeRatio, OutType(-1) * viskores::Infinity<OutType>());
}


// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          CellShapeType shape,
                                          viskores::ErrorCode&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(0.0);
}

// ========================= 2D cells ==================================


// Compute the edge ratio of a line.
// Formula: Maximum edge length divided by minimum edge length
// Trivially equals 1, since only a single edge
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagLine,
                                          viskores::ErrorCode& ec)
{
  UNUSED(pts);
  if (numPts < 2)
  {
    ec = viskores::ErrorCode::InvalidCellMetric;
    return OutType(0.0);
  }
  return OutType(1.0);
}

// Compute the edge ratio of a triangle.
// Formula: Maximum edge length divided by minimum edge length
// Equals 1 for an equilateral unit triangle
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagTriangle,
                                          viskores::ErrorCode& ec)
{
  if (numPts != 3)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  const viskores::IdComponent numEdges = 3; //pts.GetNumberOfComponents();

  //The 3 edges of a triangle
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge TriEdges[3] = { pts[1] - pts[0], pts[2] - pts[1], pts[0] - pts[2] };
  return viskores::worklet::cellmetrics::ComputeEdgeRatio<OutType>(
    viskores::make_VecC(TriEdges, numEdges));
}


// Compute the edge ratio of a quadrilateral.
// Formula: Maximum edge length divided by minimum edge length
// Equals 1 for a unit square
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagQuad,
                                          viskores::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  viskores::IdComponent numEdges = 4; //pts.GetNumberOfComponents();

  //The 4 edges of a quadrilateral
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge QuadEdges[4] = { pts[1] - pts[0], pts[2] - pts[1], pts[3] - pts[2], pts[0] - pts[3] };

  return viskores::worklet::cellmetrics::ComputeEdgeRatio<OutType>(
    viskores::make_VecC(QuadEdges, numEdges));
}



// ============================= 3D Volume cells ==================================i

// Compute the edge ratio of a tetrahedron.
// Formula: Maximum edge length divided by minimum edge length
// Equals 1 for a unit equilateral tetrahedron
// Acceptable range: [1,3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagTetra,
                                          viskores::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  viskores::IdComponent numEdges = 6; //pts.GetNumberOfComponents();

  //The 6 edges of a tetrahedron
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge TetEdges[6] = { pts[1] - pts[0], pts[2] - pts[1], pts[0] - pts[2],
                             pts[3] - pts[0], pts[3] - pts[1], pts[3] - pts[2] };

  return viskores::worklet::cellmetrics::ComputeEdgeRatio<OutType>(
    viskores::make_VecC(TetEdges, numEdges));
}


// Compute the edge ratio of a hexahedron.
// Formula: Maximum edge length divided by minimum edge length
// Equals 1 for a unit cube
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagHexahedron,
                                          viskores::ErrorCode& ec)
{
  if (numPts != 8)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  viskores::IdComponent numEdges = 12; //pts.GetNumberOfComponents();

  //The 12 edges of a hexahedron
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge HexEdges[12] = { pts[1] - pts[0], pts[2] - pts[1], pts[3] - pts[2], pts[0] - pts[3],
                              pts[5] - pts[4], pts[6] - pts[5], pts[7] - pts[6], pts[4] - pts[7],
                              pts[4] - pts[0], pts[5] - pts[1], pts[6] - pts[2], pts[7] - pts[3] };

  return viskores::worklet::cellmetrics::ComputeEdgeRatio<OutType>(
    viskores::make_VecC(HexEdges, numEdges));
}


// Compute the edge ratio of a wedge/prism.
// Formula: Maximum edge length divided by minimum edge length
// Equals 1 for a right unit wedge
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagWedge,
                                          viskores::ErrorCode& ec)
{
  if (numPts != 6)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  viskores::IdComponent numEdges = 9; //pts.GetNumberOfComponents();

  //The 9 edges of a wedge/prism
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge WedgeEdges[9] = { pts[1] - pts[0], pts[2] - pts[1], pts[0] - pts[2],
                               pts[4] - pts[3], pts[5] - pts[4], pts[3] - pts[5],
                               pts[3] - pts[0], pts[4] - pts[1], pts[5] - pts[2] };

  return viskores::worklet::cellmetrics::ComputeEdgeRatio<OutType>(
    viskores::make_VecC(WedgeEdges, numEdges));
}

// Compute the edge ratio of a pyramid.
// Formula: Maximum edge length divided by minimum edge length
// TODO: Equals 1 for a right unit (square base?) pyramid (?)
// Full range: [1,FLOAT_MAX]
// TODO: Verdict/VTK don't define this metric for a pyramid. What does VisIt output?
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellEdgeRatioMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagPyramid,
                                          viskores::ErrorCode& ec)
{
  if (numPts != 5)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  viskores::IdComponent numEdges = 8; // pts.GetNumberOfComponents();

  //The 8 edges of a pyramid (4 quadrilateral base edges + 4 edges to apex)
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge PyramidEdges[8] = {
    pts[1] - pts[0], pts[2] - pts[1], pts[2] - pts[3], pts[3] - pts[0],
    pts[4] - pts[0], pts[4] - pts[1], pts[4] - pts[2], pts[4] - pts[3]
  };

  return viskores::worklet::cellmetrics::ComputeEdgeRatio<OutType>(
    viskores::make_VecC(PyramidEdges, numEdges));
}



} // namespace cellmetrics
} // namespace worklet
} // namespace viskores

#endif // viskores_worklet_cellmetrics_CellEdgeRatioMetric_h
