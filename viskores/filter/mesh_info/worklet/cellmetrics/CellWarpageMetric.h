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
#ifndef viskores_worklet_CellWarpageMetric_h
#define viskores_worklet_CellWarpageMetric_h

#include "TypeOfCellQuadrilateral.h"
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
VISKORES_EXEC OutType CellWarpageMetric(const viskores::IdComponent& viskoresNotUsed(numPts),
                                        const PointCoordVecType& viskoresNotUsed(pts),
                                        CellShapeType viskoresNotUsed(shape),
                                        viskores::ErrorCode& viskoresNotUsed(ec))
{
  //ec = viskores::ErrorCode::InvalidCellMetric;
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellWarpageMetric(const viskores::IdComponent& viskoresNotUsed(numPts),
                                        const PointCoordVecType& pts,
                                        viskores::CellShapeTagQuad,
                                        viskores::ErrorCode& viskoresNotUsed(ec))
{
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Vector N0Mag = GetQuadN0Normalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N1Mag = GetQuadN1Normalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N2Mag = GetQuadN2Normalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N3Mag = GetQuadN3Normalized<Scalar, Vector, CollectionOfPoints>(pts);

  if (N0Mag < Scalar(0.0) || N1Mag < Scalar(0.0) || N2Mag < Scalar(0.0) || N3Mag < Scalar(0.0))
    return viskores::Infinity<Scalar>();
  const Scalar n0dotn2 = static_cast<Scalar>(viskores::Dot(N0Mag, N2Mag));
  const Scalar n1dotn3 = static_cast<Scalar>(viskores::Dot(N1Mag, N3Mag));
  const Scalar min = viskores::Min(n0dotn2, n1dotn3);

  const Scalar minCubed = viskores::Pow(min, 3);
  //return Scalar(1.0) - minCubed; // AS DEFINED IN THE MANUAL
  return minCubed; // AS DEFINED IN VISIT SOURCE CODE
}
}
} // worklet
} // viskores
#endif // viskores_worklet_CellWarpageMetric_h
