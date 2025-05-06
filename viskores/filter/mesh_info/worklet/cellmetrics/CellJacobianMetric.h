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
#ifndef viskores_worklet_cellmetrics_Jacobian_h
#define viskores_worklet_cellmetrics_Jacobian_h

/*
 * Mesh quality metric functions that computes the Jacobian of mesh cells.
 *
 * These metric computations are adapted from the VTK implementation of the Verdict library,
 * which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
 * of mesh spaces.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
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

// By default, cells return the metric 0.0 unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellJacobianMetric(const viskores::IdComponent& numPts,
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

// Compute the Jacobian of a quadrilateral.
// Formula: min{Jacobian at each vertex}
// Equals 1 for a unit square
// Acceptable range: [0,FLOAT_MAX]
// Normal range: [0,FLOAT_MAX]
// Full range: [FLOAT_MIN,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellJacobianMetric(const viskores::IdComponent& numPts,
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

  const Scalar alpha0 = GetQuadAlpha0<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha1 = GetQuadAlpha1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha2 = GetQuadAlpha2<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar alpha3 = GetQuadAlpha3<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar q = viskores::Min(alpha0, viskores::Min(alpha1, viskores::Min(alpha2, alpha3)));

  return q;
}

// ============================= 3D Volume cells ==================================
// Compute the Jacobian of a hexahedron.
// Formula: min{ {Alpha_i for i in 1..7}, Alpha_8/64}
//  -Alpha_i -> Jacobian determinant at respective vertex
//  -Alpha_8 -> Jacobian at center
// Equals 1 for a unit cube
// Acceptable Range: [0, FLOAT_MAX]
// Normal Range: [0, FLOAT_MAX]
// Full range: [FLOAT_MIN ,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellJacobianMetric(const viskores::IdComponent& numPts,
                                         const PointCoordVecType& pts,
                                         viskores::CellShapeTagHexahedron,
                                         viskores::ErrorCode& ec)
{
  if (numPts != 8)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar alpha0 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(0));
  const Scalar alpha1 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(1));
  const Scalar alpha2 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(2));
  const Scalar alpha3 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(3));
  const Scalar alpha4 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(4));
  const Scalar alpha5 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(5));
  const Scalar alpha6 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(6));
  const Scalar alpha7 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(7));
  const Scalar alpha8 = GetHexAlphai<Scalar, Vector, CollectionOfPoints>(pts, viskores::Id(8));
  const Scalar alpha8Div64 = alpha8 / Scalar(64.0);

  const Scalar q = viskores::Min(
    alpha0,
    viskores::Min(
      alpha1,
      viskores::Min(
        alpha2,
        viskores::Min(
          alpha3,
          viskores::Min(
            alpha4,
            viskores::Min(alpha5, viskores::Min(alpha6, viskores::Min(alpha7, alpha8Div64))))))));

  return q;
}

// Compute the Jacobian of a tetrahedron.
// Formula: (L2 x L0) * L3
// Equals Sqrt(2) / 2 for unit equilateral tetrahedron
// Acceptable Range: [0, FLOAT_MAX]
// Normal Range: [0, FLOAT_MAX]
// Full range: [FLOAT_MIN,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellJacobianMetric(const viskores::IdComponent& numPts,
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

  const Vector L0 = GetTetraL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L2 = GetTetraL2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L3 = GetTetraL3<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar q = static_cast<Scalar>((viskores::Dot(viskores::Cross(L2, L0), L3)));

  return q;
}
} // namespace cellmetrics
} // namespace worklet
} // namespace viskores

#endif // viskores_worklet_cellmetrics_Jacobian_h
