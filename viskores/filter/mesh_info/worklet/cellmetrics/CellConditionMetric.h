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
#ifndef viskores_worklet_cellmetrics_CellConditionMetric_h
#define viskores_worklet_cellmetrics_CellConditionMetric_h

/*
* Mesh quality metric functions that compute the condition metric of mesh cells.
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
*/

#include "CellMaxAspectFrobeniusMetric.h"
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
VISKORES_EXEC OutType CellConditionMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          CellShapeType shape,
                                          viskores::ErrorCode&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(0);
}

// =============================== Condition metric cells ==================================

// Compute the condition quality metric of a triangular cell.
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellConditionMetric(const viskores::IdComponent& numPts,
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

  const Scalar area = GetTriangleArea<Scalar, Vector, CollectionOfPoints>(pts);

  if (area == Scalar(0.0))
  {
    return viskores::Infinity<Scalar>();
  }
  const Scalar two(2.0);
  const Scalar rootThree = viskores::Sqrt(Scalar(3.0));
  const Vector L1 = GetTriangleL1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L2 = GetTriangleL2<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar q =
    static_cast<Scalar>((viskores::Dot(L2, L2) + viskores::Dot(L1, L1) + viskores::Dot(L1, L2))) /
    (two * area * rootThree);
  return q;
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellConditionMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagQuad,
                                          viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(ec);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar a0 = GetQuadAlpha0<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar a1 = GetQuadAlpha1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar a2 = GetQuadAlpha2<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar a3 = GetQuadAlpha3<Scalar, Vector, CollectionOfPoints>(pts);

  if (a0 < viskores::NegativeInfinity<Scalar>() || a1 < viskores::NegativeInfinity<Scalar>() ||
      a2 < viskores::NegativeInfinity<Scalar>() || a3 < viskores::NegativeInfinity<Scalar>())
  {
    return viskores::Infinity<Scalar>();
  }

  const Scalar l0 = GetQuadL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetQuadL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetQuadL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l3 = GetQuadL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar hhalf(0.5);

  const Scalar q0 = ((l0 * l0) + (l3 * l3)) / a0;
  const Scalar q1 = ((l1 * l1) + (l0 * l0)) / a1;
  const Scalar q2 = ((l2 * l2) + (l1 * l1)) / a2;
  const Scalar q3 = ((l3 * l3) + (l2 * l2)) / a3;

  const Scalar q = hhalf * viskores::Max(q0, viskores::Max(q1, viskores::Max(q2, q3)));
  return q;
}

// ============================= 3D volumetric cells ==================================
/// Compute the condition metric of a tetrahedron.
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellConditionMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagTetra,
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

  const Scalar negTwo(-2.0);
  const Scalar three(3.0);
  const Scalar root3 = viskores::Sqrt(three);
  const Scalar root6 = viskores::Sqrt(Scalar(6.0));
  const Vector L0 = GetTetraL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L2 = GetTetraL2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L3 = GetTetraL3<Scalar, Vector, CollectionOfPoints>(pts);

  const Vector C1 = L0;
  const Vector C2 = ((negTwo * L2) - L0) / root3;
  const Vector C3 = ((three * L3) + L2 - L0) / root6;

  const Scalar cDet = static_cast<Scalar>(viskores::Dot(C1, viskores::Cross(C2, C3)));

  if (cDet <= Scalar(0.0))
  {
    return viskores::Infinity<Scalar>();
  }

  const Vector C1xC2 = viskores::Cross(C1, C2);
  const Vector C2xC3 = viskores::Cross(C2, C3);
  const Vector C1xC3 = viskores::Cross(C1, C3);

  const Scalar t1 =
    static_cast<Scalar>(viskores::Dot(C1, C1) + viskores::Dot(C2, C2) + viskores::Dot(C3, C3));
  const Scalar t2 = static_cast<Scalar>(viskores::Dot(C1xC2, C1xC2) + viskores::Dot(C2xC3, C2xC3) +
                                        viskores::Dot(C1xC3, C1xC3));

  const Scalar q = viskores::Sqrt(t1 * t2) / (three * cDet);
  return q;
}

// Condition of a hex cell is a deprecated/legacy metric which is identical to the Max Aspect Frobenius metric.
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellConditionMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagHexahedron,
                                          viskores::ErrorCode& ec)
{
  return CellMaxAspectFrobeniusMetric<OutType, PointCoordVecType>(
    numPts, pts, viskores::CellShapeTagHexahedron(), ec);
}
}
}
}
#endif // viskores_worklet_cellmetrics_CellConditionMetric_h
