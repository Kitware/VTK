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
#ifndef viskores_worklet_cellmetrics_CellMinAngleMetric_h
#define viskores_worklet_cellmetrics_CellMinAngleMetric_h

/*
* Mesh quality metric functions that compute the minimum angle of cell in a mesh.
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
*/

#include "TypeOfCellQuadrilateral.h"
#include "TypeOfCellTriangle.h"
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

// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellMinAngleMetric(const viskores::IdComponent& numPts,
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
// Compute the minimum angle of a triangle.
// Formula: q = min( arccos((Ln dot Ln+1)/(||Ln|| * ||Ln+1||))(180º/π) for n 0,1, and 2 )
//   - L3 = L0
//   - if any edge has length 0, return q = 360º
//   - All angle measurements are in degrees
// q equals 60 for a unit triangle
// Acceptable range: [30º, 60º]
// Normal Range: [0º, 360º]
// Full range: [0º, 360º]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMinAngleMetric(const viskores::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         viskores::CellShapeTagTriangle,
                                         viskores::ErrorCode& ec)
{
  if (numPts != 3)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar l0 = GetTriangleL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetTriangleL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetTriangleL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);

  if (l0 <= Scalar(0.0) || l1 <= Scalar(0.0) || l2 <= Scalar(0.0))
  {
    return Scalar(0.0);
  }

  const Scalar oneEightyOverPi = (Scalar)57.2957795131;
  const Scalar two(2.0);
  const Scalar q0 = viskores::ACos(((l1 * l1) + (l2 * l2) - (l0 * l0)) / (two * l1 * l2));
  const Scalar q1 = viskores::ACos(((l2 * l2) + (l0 * l0) - (l1 * l1)) / (two * l2 * l0));
  const Scalar q2 = viskores::ACos(((l0 * l0) + (l1 * l1) - (l2 * l2)) / (two * l0 * l1));

  const Scalar q = viskores::Min(q0, viskores::Min(q1, q2));

  const Scalar qInDegrees = q * oneEightyOverPi;

  return qInDegrees;
}
// Compute the min angle of a quadrilateral.
// Formula: q = min( Ai for i 0,1,2, and 3 )
//   - L4 = L0
//   - Ai = -1^Si arccos(-1(Li dot Li+1)/(||Li||||Li+1||) )(180/π) + 360º*Si
//   - if ||Li|| <= FLOAT_MIN or ||Li+1|| <= FLOAT_MIN, return q = 360º
// q = 90º for a unit square
// Acceptable range: [45º, 90º]
// Normal Range: [0º, 90º]
// Full range: [0º, 360º]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMinAngleMetric(const viskores::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         viskores::CellShapeTagQuad,
                                         viskores::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar l0 = GetQuadL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetQuadL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetQuadL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l3 = GetQuadL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts);

  if (l0 <= Scalar(0.0) || l1 <= Scalar(0.0) || l2 <= Scalar(0.0) || l3 <= Scalar(0.0))
  {
    return Scalar(0.0);
  }

  const Scalar alpha0 = GetQuadAlpha0<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha1 = GetQuadAlpha1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha2 = GetQuadAlpha2<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha3 = GetQuadAlpha3<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar s0 = alpha0 < Scalar(0.0) ? Scalar(1.0) : Scalar(0.0);
  const Scalar s1 = alpha1 < Scalar(0.0) ? Scalar(1.0) : Scalar(0.0);
  const Scalar s2 = alpha2 < Scalar(0.0) ? Scalar(1.0) : Scalar(0.0);
  const Scalar s3 = alpha3 < Scalar(0.0) ? Scalar(1.0) : Scalar(0.0);

  const Vector L0 = GetQuadL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L1 = GetQuadL1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L2 = GetQuadL2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L3 = GetQuadL3<Scalar, Vector, CollectionOfPoints>(pts);

  // This angle is given in degrees, not radians. The verdict definition (1) converts to degrees, (2) gives co(terminal) angles, and (3) takes the min/max.
  // Further, it combines steps 1 & 2 into a single expression using clever (-1)^power flags.
  const Scalar neg1(-1.0);
  const Scalar oneEightyOverPi = (Scalar)57.2957795131; // ~ 180/pi
  const Scalar threeSixty(360.0);
  const Scalar q0 =
    (viskores::Pow(neg1, s0) *
     viskores::ACos(neg1 * (static_cast<Scalar>(viskores::Dot(L0, L1)) / (l0 * l1))) *
     oneEightyOverPi) +
    (threeSixty * s0);
  const Scalar q1 =
    (viskores::Pow(neg1, s1) *
     viskores::ACos(neg1 * (static_cast<Scalar>(viskores::Dot(L1, L2)) / (l1 * l2))) *
     oneEightyOverPi) +
    (threeSixty * s1);
  const Scalar q2 =
    (viskores::Pow(neg1, s2) *
     viskores::ACos(neg1 * (static_cast<Scalar>(viskores::Dot(L2, L3)) / (l2 * l3))) *
     oneEightyOverPi) +
    (threeSixty * s2);
  const Scalar q3 =
    (viskores::Pow(neg1, s3) *
     viskores::ACos(neg1 * (static_cast<Scalar>(viskores::Dot(L3, L0)) / (l3 * l0))) *
     oneEightyOverPi) +
    (threeSixty * s3);

  const Scalar q = viskores::Min(q0, viskores::Min(q1, viskores::Min(q2, q3)));
  return q;
}
} // namespace cellmetrics
} // namespace worklet
} // namespace viskores
#endif // viskores_worklet_cellmetrics_CellMinAngleMetric_h
