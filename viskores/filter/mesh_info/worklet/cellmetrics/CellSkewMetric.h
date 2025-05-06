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
#ifndef viskores_worklet_CellSkewMetric_h
#define viskores_worklet_CellSkewMetric_h
/*
*/

#include "CellConditionMetric.h"
#include "TypeOfCellHexahedral.h"
#include "TypeOfCellQuadrilateral.h"
#include "TypeOfCellTetrahedral.h"
#include "TypeOfCellTriangle.h"
#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/ErrorCode.h>
#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>

namespace viskores
{
namespace worklet
{
namespace cellmetrics
{
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellSkewMetric(const viskores::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     CellShapeType shape,
                                     viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  UNUSED(ec);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellSkewMetric(const viskores::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     viskores::CellShapeTagHexahedron,
                                     viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(ec);
  using Scalar = OutType;
  using Vector = typename PointCoordVecType::ComponentType;
  Vector X1 = (pts[1] - pts[0]) + (pts[2] - pts[3]) + (pts[5] - pts[4]) + (pts[6] - pts[7]);
  auto X1_mag = viskores::Magnitude(X1);
  if (Scalar(X1_mag) <= Scalar(0.0))
    return viskores::Infinity<Scalar>();
  Vector x1 = X1 / X1_mag;
  Vector X2 = (pts[3] - pts[0]) + (pts[2] - pts[1]) + (pts[7] - pts[4]) + (pts[6] - pts[5]);
  auto X2_mag = viskores::Magnitude(X2);
  if (Scalar(X2_mag) <= Scalar(0.0))
    return viskores::Infinity<Scalar>();
  Vector x2 = X2 / X2_mag;
  Vector X3 = (pts[4] - pts[0]) + (pts[5] - pts[1]) + (pts[6] - pts[2]) + (pts[7] - pts[3]);
  auto X3_mag = viskores::Magnitude(X3);
  if (Scalar(X3_mag) <= Scalar(0.0))
    return viskores::Infinity<Scalar>();
  Vector x3 = X3 / X3_mag;
  return static_cast<Scalar>(viskores::Max(
    viskores::Dot(x1, x2), viskores::Max(viskores::Dot(x1, x3), viskores::Dot(x2, x3))));
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellSkewMetric(const viskores::IdComponent& numPts,
                                     const PointCoordVecType& pts,
                                     viskores::CellShapeTagQuad,
                                     viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(ec);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;
  const Vector X0 = GetQuadX0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector X1 = GetQuadX1<Scalar, Vector, CollectionOfPoints>(pts);
  const auto X0Mag = viskores::Magnitude(X0);
  const auto X1Mag = viskores::Magnitude(X1);

  if (Scalar(X0Mag) < Scalar(0.0) || Scalar(X1Mag) < Scalar(0.0))
    return Scalar(0.0);
  const Vector x0Normalized = X0 / X0Mag;
  const Vector x1Normalized = X1 / X1Mag;
  const Scalar dot = static_cast<Scalar>(viskores::Dot(x0Normalized, x1Normalized));
  return viskores::Abs(dot);
}
}
} // worklet
} // viskores
#endif
