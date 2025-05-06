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
#ifndef viskores_worklet_cellmetrics_Oddy_h
#define viskores_worklet_cellmetrics_Oddy_h

/*
* Mesh quality metric functions that compute the Oddy of mesh cells.
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
*/

#include "TypeOfCellHexahedral.h"
#include "TypeOfCellQuadrilateral.h"
#include "TypeOfCellTetrahedral.h"
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
VISKORES_EXEC OutType CellOddyMetric(const viskores::IdComponent& numPts,
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
// Compute the Oddy of a quadrilateral.
// Formula: for i 0 to 3: max{[(||Li||^2 - ||Li+1||^2)^2 + 4((Li * Li+1)^2)] / (2||Ni+1||^2)}
//   - L4 = L0
//   - '*' symbolizes the dot product of two vectors
//   - Ni is the normal vector associated with each point
// Equals 0 for a unit square
// Acceptable range: [0,0.5]
// Normal range: [0,FLOAT_MAX]
// Full range: [0,FLOAT_MAX]
// Note, for loop avoided because L4 = L0
template <typename Scalar, typename Vector>
VISKORES_EXEC Scalar GetQuadOddyQi(const Vector& Li, const Vector& LiPlus1, const Vector& NiPlus1)
{
  const Scalar two(2.0);
  const Scalar four(4.0);
  const Scalar liMagnitudeSquared = static_cast<Scalar>(viskores::MagnitudeSquared(Li));
  const Scalar liPlus1MagnitudeSquared = static_cast<Scalar>(viskores::MagnitudeSquared(LiPlus1));
  const Scalar niPlus1MagnitudeSquared = static_cast<Scalar>(viskores::MagnitudeSquared(NiPlus1));
  const Scalar q =
    (((liMagnitudeSquared - liPlus1MagnitudeSquared) *
      (liMagnitudeSquared - liPlus1MagnitudeSquared)) +
     (four * static_cast<Scalar>(viskores::Dot(Li, LiPlus1) * viskores::Dot(Li, LiPlus1)))) /
    (two * niPlus1MagnitudeSquared);

  return q;
}
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellOddyMetric(const viskores::IdComponent& numPts,
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

  const Vector L0 = GetQuadL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L1 = GetQuadL1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L2 = GetQuadL2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L3 = GetQuadL3<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N0 = GetQuadN0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N1 = GetQuadN1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N2 = GetQuadN2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N3 = GetQuadN3<Scalar, Vector, CollectionOfPoints>(pts);

  if (viskores::MagnitudeSquared(N0) <= Scalar(0.0) ||
      viskores::MagnitudeSquared(N1) <= Scalar(0.0) ||
      viskores::MagnitudeSquared(N2) <= Scalar(0.0) ||
      viskores::MagnitudeSquared(N3) <= Scalar(0.0))
  {
    return viskores::Infinity<Scalar>();
  }

  const Scalar q0 = GetQuadOddyQi<Scalar, Vector>(L0, L1, N1);
  const Scalar q1 = GetQuadOddyQi<Scalar, Vector>(L1, L2, N2);
  const Scalar q2 = GetQuadOddyQi<Scalar, Vector>(L2, L3, N3);
  const Scalar q3 = GetQuadOddyQi<Scalar, Vector>(L3, L0, N0);

  const Scalar q = viskores::Max(q0, viskores::Max(q1, viskores::Max(q2, q3)));
  return q;
}

// ============================= 3D Volume cells ==================================
// Compute the Oddy of a hexahedron.
// Formula:
// Equals 0 for a unit cube
// Acceptable Range: [0, 0.5]
// Normal range: [0,FLOAT_MAX]
// Full range: [0,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellOddyMetric(const viskores::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     viskores::CellShapeTagHexahedron,
                                     viskores::ErrorCode& ec)
{
  if (numPts != 8)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  //The 12 points of a hexahedron
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge HexEdges[12] = {
    pts[1] - pts[0], // 0
    pts[2] - pts[1], pts[3] - pts[2],
    pts[3] - pts[0], // 3
    pts[4] - pts[0], pts[5] - pts[1],
    pts[6] - pts[2], // 6
    pts[7] - pts[3], pts[5] - pts[4],
    pts[6] - pts[5], // 9
    pts[7] - pts[6],
    pts[7] - pts[4] // 11
  };

  Edge principleXAxis = HexEdges[0] + (pts[2] - pts[3]) + HexEdges[8] + (pts[6] - pts[7]);
  Edge principleYAxis = (pts[3] - pts[0]) + HexEdges[1] + (pts[7] - pts[4]) + HexEdges[9];
  Edge principleZAxis = HexEdges[4] + HexEdges[5] + HexEdges[6] + HexEdges[7];
  Edge hexJacobianMatrices[9][3] = {
    { HexEdges[0], HexEdges[3], HexEdges[4] },
    { HexEdges[1], (-1 * HexEdges[0]), HexEdges[5] },
    { HexEdges[2], (-1 * HexEdges[1]), HexEdges[6] },
    { (-1 * HexEdges[3]), (-1 * HexEdges[2]), HexEdges[7] },
    { HexEdges[11], HexEdges[8], (-1 * HexEdges[4]) },
    { (-1 * HexEdges[8]), HexEdges[9], (-1 * HexEdges[5]) },
    { (-1 * HexEdges[9]), HexEdges[10], (-1 * HexEdges[6]) },
    { (-1 * HexEdges[10]), (-1 * HexEdges[11]), (-1 * HexEdges[7]) },
    { principleXAxis, principleYAxis, principleZAxis }
  };

  OutType third = (OutType)(1.0 / 3.0);
  OutType negativeInfinity = viskores::NegativeInfinity<OutType>();
  OutType tempMatrix1_1, tempMatrix1_2, tempMatrix1_3, tempMatrix2_2, tempMatrix2_3, tempMatrix3_3,
    determinant;
  OutType firstPartOfNumerator, secondPartOfNumerator, currentOddy, maxOddy = negativeInfinity;
  for (viskores::IdComponent matrixNumber = 0; matrixNumber < 9; matrixNumber++)
  {
    /*
    // these computations equal the value at X_Y for the matrix B,
    // where B = matrix A multiplied by its transpose.
    // the values at X_X are also used for the matrix multiplication AA.
    // Note that the values 1_2 = 2_1, 1_3 = 3_1, and 2_3 = 3_2.
    // This fact is used to optimize the computation
    */
    tempMatrix1_1 = static_cast<OutType>(
      viskores::Dot(hexJacobianMatrices[matrixNumber][0], hexJacobianMatrices[matrixNumber][0]));
    tempMatrix1_2 = static_cast<OutType>(
      viskores::Dot(hexJacobianMatrices[matrixNumber][0], hexJacobianMatrices[matrixNumber][1]));
    tempMatrix1_3 = static_cast<OutType>(
      viskores::Dot(hexJacobianMatrices[matrixNumber][0], hexJacobianMatrices[matrixNumber][2]));
    tempMatrix2_2 = static_cast<OutType>(
      viskores::Dot(hexJacobianMatrices[matrixNumber][1], hexJacobianMatrices[matrixNumber][1]));
    tempMatrix2_3 = static_cast<OutType>(
      viskores::Dot(hexJacobianMatrices[matrixNumber][1], hexJacobianMatrices[matrixNumber][2]));
    tempMatrix3_3 = static_cast<OutType>(
      viskores::Dot(hexJacobianMatrices[matrixNumber][2], hexJacobianMatrices[matrixNumber][2]));
    determinant = static_cast<OutType>(viskores::Dot(
      hexJacobianMatrices[matrixNumber][0],
      viskores::Cross(hexJacobianMatrices[matrixNumber][1], hexJacobianMatrices[matrixNumber][2])));
    if (determinant <= OutType(0.0))
    {
      return viskores::Infinity<OutType>();
    }
    else
    {
      firstPartOfNumerator = (tempMatrix1_1 * tempMatrix1_1) + 2 * (tempMatrix1_2 * tempMatrix1_2) +
        2 * (tempMatrix1_3 * tempMatrix1_3) + (tempMatrix2_2 * tempMatrix2_2) +
        2 * (tempMatrix2_3 * tempMatrix2_3) + (tempMatrix3_3 * tempMatrix3_3);
      secondPartOfNumerator = tempMatrix1_1 + tempMatrix2_2 + tempMatrix3_3;
      secondPartOfNumerator *= secondPartOfNumerator;
      secondPartOfNumerator *= third;
      currentOddy = (firstPartOfNumerator - secondPartOfNumerator) /
        (viskores::Pow(determinant, OutType(4.0 * third)));
      maxOddy = currentOddy > maxOddy ? currentOddy : maxOddy;
    }
  }
  if (maxOddy > 0)
  {
    return viskores::Min(maxOddy, viskores::Infinity<OutType>()); //normal case
  }

  return viskores::Max(maxOddy, negativeInfinity);
}
} // namespace cellmetrics
} // namespace worklet
} // namespace viskores
#endif // viskores_worklet_cellmetrics_Oddy_h
