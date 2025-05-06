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
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_worklet_cellmetrics_CellDiagonalRatioMetric_h
#define viskores_worklet_cellmetrics_CellDiagonalRatioMetric_h

/*
* Mesh quality metric functions that compute the diagonal ratio of mesh cells.
* The diagonal ratio of a cell is defined as the length (magnitude) of the longest
* cell diagonal length divided by the length of the shortest cell diagonal length.
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** The edge ratio computations for a pyramid cell types is not defined in the
* VTK implementation, but is provided here.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
*/

#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/ErrorCode.h>
#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>

#define UNUSED(expr) (void)(expr);

namespace viskores
{
namespace worklet
{
namespace cellmetrics
{

using FloatType = viskores::FloatDefault;

template <typename OutType, typename VecType>
VISKORES_EXEC inline OutType ComputeDiagonalRatio(const VecType& diagonals)
{
  const viskores::Id numDiagonals = diagonals.GetNumberOfComponents();

  //Compare diagonal lengths to determine the longest and shortest
  //TODO: Could we use lambda expression here?

  FloatType d0Len = (FloatType)viskores::MagnitudeSquared(diagonals[0]);
  FloatType currLen, minLen = d0Len, maxLen = d0Len;
  for (int i = 1; i < numDiagonals; i++)
  {
    currLen = (FloatType)viskores::MagnitudeSquared(diagonals[i]);
    if (currLen < minLen)
      minLen = currLen;
    if (currLen > maxLen)
      maxLen = currLen;
  }

  if (maxLen <= OutType(0.0))
    return viskores::Infinity<OutType>();

  //Take square root because we only did magnitude squared before
  OutType diagonalRatio = (OutType)viskores::Sqrt(minLen / maxLen);
  return diagonalRatio;
}

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellDiagonalRatioMetric(const viskores::IdComponent& numPts,
                                              const PointCoordVecType& pts,
                                              CellShapeType shape,
                                              viskores::ErrorCode&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(-1.0);
}

// ========================= 2D cells ==================================
// Compute the diagonal ratio of a quadrilateral.
// Formula: Maximum diagonal length divided by minimum diagonal length
// Equals 1 for a unit square
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellDiagonalRatioMetric(const viskores::IdComponent& numPts,
                                              const PointCoordVecType& pts,
                                              viskores::CellShapeTagQuad,
                                              viskores::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  viskores::IdComponent numDiagonals = 2; //pts.GetNumberOfComponents();

  //The 2 diagonals of a quadrilateral
  using Diagonal = typename PointCoordVecType::ComponentType;
  const Diagonal QuadDiagonals[2] = { pts[2] - pts[0], pts[3] - pts[1] };

  return viskores::worklet::cellmetrics::ComputeDiagonalRatio<OutType>(
    viskores::make_VecC(QuadDiagonals, numDiagonals));
}

// ============================= 3D Volume cells ==================================
// Compute the diagonal ratio of a hexahedron.
// Formula: Maximum diagonal length divided by minimum diagonal length
// Equals 1 for a unit cube
// Acceptable Range: [0.65, 1]
// Normal Range: [0, 1]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellDiagonalRatioMetric(const viskores::IdComponent& numPts,
                                              const PointCoordVecType& pts,
                                              viskores::CellShapeTagHexahedron,
                                              viskores::ErrorCode& ec)
{
  if (numPts != 8)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  viskores::IdComponent numDiagonals = 4; //pts.GetNumberOfComponents();

  //The 4 diagonals of a hexahedron
  using Diagonal = typename PointCoordVecType::ComponentType;
  const Diagonal HexDiagonals[4] = {
    pts[6] - pts[0], pts[7] - pts[1], pts[4] - pts[2], pts[5] - pts[3]
  };

  return viskores::worklet::cellmetrics::ComputeDiagonalRatio<OutType>(
    viskores::make_VecC(HexDiagonals, numDiagonals));
}
} // namespace cellmetrics
} // namespace worklet
} // namespace viskores
#endif // viskores_worklet_cellmetrics_CellDiagonalRatioMetric_h
