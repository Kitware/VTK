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
#ifndef viskores_worklet_cellmetrics_CellRelativeSizeSquaredMetric_h
#define viskores_worklet_cellmetrics_CellRelativeSizeSquaredMetric_h

/*
 * Mesh quality metric functions that compute the relative size squared of mesh
 * cells. The RSS of a cell is defined as the square of the minimum of: the area
 * divided by the average area of an ensemble of triangles or the inverse. For
 * 3D cells we use the volumes instead of the areas.
 *
 * These metric computations are adapted from the VTK implementation of the
 * Verdict library, which provides a set of mesh/cell metrics for evaluating the
 * geometric qualities of regions of mesh spaces.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
 * See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this
 * metric)
 */

#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/Matrix.h>
#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/exec/CellMeasure.h>

#define UNUSED(expr) (void)(expr);

namespace viskores
{
namespace worklet
{
namespace cellmetrics
{

using FloatType = viskores::FloatDefault;

// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellRelativeSizeSquaredMetric(const viskores::IdComponent& numPts,
                                                    const PointCoordVecType& pts,
                                                    const OutType& avgArea,
                                                    CellShapeType shape,
                                                    viskores::ErrorCode&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(avgArea);
  UNUSED(shape);
  return OutType(-1.);
}

// ========================= 2D cells ==================================

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellRelativeSizeSquaredMetric(const viskores::IdComponent& numPts,
                                                    const PointCoordVecType& pts,
                                                    const OutType& avgArea,
                                                    viskores::CellShapeTagTriangle tag,
                                                    viskores::ErrorCode& ec)
{
  UNUSED(ec);
  if (numPts != 3)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(-1.);
  }
  OutType A = viskores::exec::CellMeasure<OutType>(numPts, pts, tag, ec);
  OutType R = A / avgArea;
  if (R == OutType(0.))
    return OutType(0.);
  OutType q = viskores::Pow(viskores::Min(R, OutType(1.) / R), OutType(2.));
  return OutType(q);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellRelativeSizeSquaredMetric(const viskores::IdComponent& numPts,
                                                    const PointCoordVecType& pts,
                                                    const OutType& avgArea,
                                                    viskores::CellShapeTagQuad tag,
                                                    viskores::ErrorCode& ec)
{
  UNUSED(ec);
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(-1.);
  }
  OutType A = viskores::exec::CellMeasure<OutType>(numPts, pts, tag, ec);
  OutType R = A / avgArea;
  if (R == OutType(0.))
    return OutType(0.);
  OutType q = viskores::Pow(viskores::Min(R, OutType(1.) / R), OutType(2.));
  return OutType(q);
}

// ========================= 3D cells ==================================

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellRelativeSizeSquaredMetric(const viskores::IdComponent& numPts,
                                                    const PointCoordVecType& pts,
                                                    const OutType& avgVolume,
                                                    viskores::CellShapeTagTetra tag,
                                                    viskores::ErrorCode& ec)
{
  UNUSED(ec);
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(-1.);
  }
  OutType V = viskores::exec::CellMeasure<OutType>(numPts, pts, tag, ec);
  OutType R = V / avgVolume;
  if (R == OutType(0.))
    return OutType(0.);
  OutType q = viskores::Pow(viskores::Min(R, OutType(1.) / R), OutType(2.));
  return OutType(q);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellRelativeSizeSquaredMetric(const viskores::IdComponent& numPts,
                                                    const PointCoordVecType& pts,
                                                    const OutType& avgVolume,
                                                    viskores::CellShapeTagHexahedron tag,
                                                    viskores::ErrorCode& ec)
{
  UNUSED(tag);
  UNUSED(ec);
  if (numPts != 8)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(-1.);
  }
  OutType X1x = static_cast<OutType>((pts[1][0] - pts[0][0]) + (pts[2][0] - pts[3][0]) +
                                     (pts[5][0] - pts[4][0]) + (pts[6][0] - pts[7][0]));
  OutType X1y = static_cast<OutType>((pts[1][1] - pts[0][1]) + (pts[2][1] - pts[3][1]) +
                                     (pts[5][1] - pts[4][1]) + (pts[6][1] - pts[7][1]));
  OutType X1z = static_cast<OutType>((pts[1][2] - pts[0][2]) + (pts[2][2] - pts[3][2]) +
                                     (pts[5][2] - pts[4][2]) + (pts[6][2] - pts[7][2]));

  OutType X2x = static_cast<OutType>((pts[2][0] - pts[0][0]) + (pts[2][0] - pts[1][0]) +
                                     (pts[7][0] - pts[4][0]) + (pts[6][0] - pts[5][0]));
  OutType X2y = static_cast<OutType>((pts[2][1] - pts[0][1]) + (pts[2][1] - pts[1][1]) +
                                     (pts[7][1] - pts[4][1]) + (pts[6][1] - pts[5][1]));
  OutType X2z = static_cast<OutType>((pts[2][2] - pts[0][2]) + (pts[2][2] - pts[1][2]) +
                                     (pts[7][2] - pts[4][2]) + (pts[6][2] - pts[5][2]));

  OutType X3x = static_cast<OutType>((pts[4][0] - pts[0][0]) + (pts[5][0] - pts[1][0]) +
                                     (pts[6][0] - pts[2][0]) + (pts[7][0] - pts[3][0]));
  OutType X3y = static_cast<OutType>((pts[4][1] - pts[0][1]) + (pts[5][1] - pts[1][1]) +
                                     (pts[6][1] - pts[2][1]) + (pts[7][1] - pts[3][1]));
  OutType X3z = static_cast<OutType>((pts[4][2] - pts[0][2]) + (pts[5][2] - pts[1][2]) +
                                     (pts[6][2] - pts[2][2]) + (pts[7][2] - pts[3][2]));
  viskores::Matrix<OutType, 3, 3> A8;
  viskores::MatrixSetRow(A8, 0, viskores::Vec<OutType, 3>(X1x, X1y, X1z));
  viskores::MatrixSetRow(A8, 1, viskores::Vec<OutType, 3>(X2x, X2y, X2z));
  viskores::MatrixSetRow(A8, 2, viskores::Vec<OutType, 3>(X3x, X3y, X3z));
  OutType D = viskores::MatrixDeterminant(A8);
  D = D / (OutType(64.) * avgVolume);
  if (D == OutType(0.))
    return OutType(0.);
  OutType q = viskores::Pow(viskores::Min(D, OutType(1.) / D), OutType(2.));
  return OutType(q);
}

} // namespace cellmetrics
} // namespace worklet
} // namespace viskores

#endif // viskores_worklet_cellmetrics_CellRelativeSizeSquaredMetric_h
