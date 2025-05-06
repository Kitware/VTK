//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
#ifndef viskores_worklet_cellmetrics_CellShapeAndSizeMetric_h
#define viskores_worklet_cellmetrics_CellShapeAndSizeMetric_h
/*
 * Mesh quality metric functions that compute the shape and size of a cell. This
 * takes the shape metric and multiplies it by the relative size squared metric.
 *
 * These metric computations are adapted from the VTK implementation of the
 * Verdict library, which provides a set of mesh/cell metrics for evaluating the
 * geometric qualities of regions of mesh spaces.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
 * See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this
 * metric)
 */

#include "CellRelativeSizeSquaredMetric.h"
#include "CellShapeMetric.h"
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

// ========================= Unsupported cells ==================================

// By default, cells have zero shape unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellShapeAndSizeMetric(const viskores::IdComponent& numPts,
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
VISKORES_EXEC OutType CellShapeAndSizeMetric(const viskores::IdComponent& numPts,
                                             const PointCoordVecType& pts,
                                             const OutType& avgArea,
                                             viskores::CellShapeTagTriangle tag,
                                             viskores::ErrorCode& ec)
{
  OutType rss = viskores::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgArea, tag, ec);
  OutType shape = viskores::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, ec);
  OutType q = rss * shape;
  return OutType(q);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellShapeAndSizeMetric(const viskores::IdComponent& numPts,
                                             const PointCoordVecType& pts,
                                             const OutType& avgArea,
                                             viskores::CellShapeTagQuad tag,
                                             viskores::ErrorCode& ec)
{
  OutType rss = viskores::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgArea, tag, ec);
  OutType shape = viskores::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, ec);
  OutType q = rss * shape;
  return OutType(q);
}

// ========================= 3D cells ==================================

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellShapeAndSizeMetric(const viskores::IdComponent& numPts,
                                             const PointCoordVecType& pts,
                                             const OutType& avgVolume,
                                             viskores::CellShapeTagTetra tag,
                                             viskores::ErrorCode& ec)
{
  OutType rss = viskores::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgVolume, tag, ec);
  OutType shape = viskores::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, ec);
  OutType q = rss * shape;
  return OutType(q);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellShapeAndSizeMetric(const viskores::IdComponent& numPts,
                                             const PointCoordVecType& pts,
                                             const OutType& avgVolume,
                                             viskores::CellShapeTagHexahedron tag,
                                             viskores::ErrorCode& ec)
{
  OutType rss = viskores::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
    numPts, pts, avgVolume, tag, ec);
  OutType shape = viskores::worklet::cellmetrics::CellShapeMetric<OutType>(numPts, pts, tag, ec);
  OutType q = rss * shape;
  return OutType(q);
}

} // namespace cellmetrics
} // namespace worklet
} // namespace viskores

#endif // viskores_worklet_cellmetrics_CellShapeAndSizeMetric_h
